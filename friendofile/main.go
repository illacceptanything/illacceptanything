package main

//go:generate $GOPATH/bin/go-bindata static/

import (
	"errors"
	"fmt"
	"html"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"strings"
	"syscall"
)

var public_url = "yourwebsite.com"

func main() {
	// /files/public/ is handled by nginx.
	// Either they get a direct link to a file - which they can do without auth -
	// or they get to us, in which case they want a directory listing.
	// Anonymous users cannot get a directory listing for /public/.

	http.HandleFunc("/files/", filesDisambiguate)
	http.ListenAndServe("127.0.0.1:8076", nil)
}

func filesDisambiguate(w http.ResponseWriter, r *http.Request) {
	if r.Method == "GET" {
		browse(w, r)
	} else {
		upload(w, r)
	}
}

func err(w http.ResponseWriter, r *http.Request, err error) {
	fmt.Fprintf(w, "<html>Something went wrong.<br>Error: %v<br>Use the back button, go back to <a href='/'>"+public_url+"</a>, or go to <a href='/files/'>the top directory</a>.</html>", html.EscapeString(err.Error()))
}

func browse(w http.ResponseWriter, r *http.Request) {
	path := r.URL.Path
	if !strings.HasSuffix(path, "/") {
		path += "/"
	}

	if !strings.HasPrefix(path, "/files/") || strings.Contains(path, "../") || strings.Contains(path, "//") {
		fmt.Printf("wrong path start err: %v %v %v %v\n", path, strings.HasPrefix(path, "/files/"), strings.ContainsAny(path, "../"), strings.ContainsAny(path, "//"))
		err(w, r, errors.New("Path did not start with /files/. Check your privilege."))
	} else {
		// rewrite path
		subpath := path[len("/files/"):]

		dirEntries, err1 := ioutil.ReadDir("/var/www/files/" + subpath)
		if err1 != nil {
			err(w, r, err1)
		} else {
			listing := make([]*pathEntry, 0)

			for _, de := range dirEntries {
				icon := ""
				ln := strings.ToLower(de.Name())
				linkName := de.Name()
				fileSize := ""

				if de.IsDir() {
					icon = "glyphicon-folder-open"
					linkName += "/"
				} else if strings.HasSuffix(ln, ".png") || strings.HasSuffix(ln, ".jpg") || strings.HasSuffix(ln, ".gif") {
					icon = "glyphicon-picture"
				} else if strings.HasSuffix(ln, ".mp4") || strings.HasSuffix(ln, ".m4v") || strings.HasSuffix(ln, ".avi") || strings.HasSuffix(ln, ".webm") || strings.HasSuffix(ln, ".wmv") {
					icon = "glyphicon-film"
				} else if strings.HasSuffix(ln, ".mp3") || strings.HasSuffix(ln, ".ogg") || strings.HasSuffix(ln, ".wav") || strings.HasSuffix(ln, ".wma") {
					icon = "glyphicon-music"
				} else if strings.HasSuffix(ln, ".txt") || strings.HasSuffix(ln, ".sh") || strings.HasSuffix(ln, ".md") {
					icon = "glyphicon-align-left"
				} else {
					icon = "glyphicon-file"
				}

				if !de.IsDir() {
					fi, err := os.Stat("/var/www/files/" + subpath + de.Name())
					if err == nil {
						sz := fi.Size() / (1024 * 1024)
						if sz < 1 {
							fileSize = "<1M"
						} else {
							fileSize = fmt.Sprintf("%vM", sz)
						}
					}
				}

				listing = append(listing, &pathEntry{
					pathLink: "http://" + public_url + "/files/" + subpath + linkName,
					pathIcon: icon,
					pathName: de.Name(),
					fileSize: fileSize,
				})
			}

			crumbLinks := []string{
				"/files/",
			}
			crumbNames := []string{
				"/var/www/files",
			}

			crumbAccum := "/files/"
			if len(subpath) > 0 {
				for _, cr := range strings.Split(subpath, "/") {
					crumbAccum += cr + "/"
					crumbLinks = append(crumbLinks, crumbAccum)
					crumbNames = append(crumbNames, cr)
				}
			}

			var stat syscall.Statfs_t
			syscall.Statfs("/var/www/files", &stat)
			diskSpace := fmt.Sprintf("%vM", stat.Bavail*uint64(stat.Bsize)/(1024*1024))

			writeBrowsePage(w, r, diskSpace, crumbNames, crumbLinks, listing, path)
		}
	}
}

func reportJSONErr(w http.ResponseWriter, r *http.Request, err error) {
	w.Header().Set("Content-Type", "application/json")
	if err != nil {
		fmt.Println(err.Error())
		fmt.Fprintf(w, "{error: '%v'}", strings.Replace(err.Error(), "'", "\"", 1))
	} else {
		fmt.Fprint(w, "{}")
	}
}

// upload logic
func upload(w http.ResponseWriter, r *http.Request) {
	path := r.URL.Path
	if !strings.HasSuffix(path, "/") {
		path += "/"
	}

	if !strings.HasPrefix(path, "/files/") || strings.Contains(path, "../") || strings.Contains(path, "//") {
		fmt.Printf("wrong path start err: %v %v %v %v\n", path, strings.HasPrefix(path, "/files/"), strings.ContainsAny(path, "../"), strings.ContainsAny(path, "//"))
		err(w, r, errors.New("Path did not start with /files/. Check your privilege."))
	} else {
		// rewrite path
		subpath := path[len("/files/"):]

		// 8MB before we start going via disk.
		r.ParseMultipartForm(8000000)

		file, handler, err := r.FormFile("upload")
		if err != nil {
			reportJSONErr(w, r, err)
			return
		}
		defer file.Close()

		fmt.Println("Uploading " + subpath + handler.Filename + "...")
		f, err := os.Create("/var/www/files/" + subpath + handler.Filename)
		if err != nil {
			reportJSONErr(w, r, err)
			return
		}
		defer f.Close()
		io.Copy(f, file)
		reportJSONErr(w, r, nil)
	}
}

type pathEntry struct {
	pathLink string
	pathIcon string
	pathName string
	fileSize string
}

func assetAsString(s string) string {
	ass, _ := Asset(s)
	return string(ass)
}

func writeBrowsePage(w http.ResponseWriter, r *http.Request, diskSpace string, breadcrumbName []string, breadcrumbLink []string, listing []*pathEntry, uploadUrl string) {
	shtml := assetAsString("static/index.html")
	pathPart := assetAsString("static/path.htmlpart")

	breadcrumbHtml := ""
	for i, bn := range breadcrumbName {
		bn = html.EscapeString(bn)
		bl := html.EscapeString(breadcrumbLink[i])

		if i < len(breadcrumbName)-1 {
			breadcrumbHtml += "	    <li><a href='" + bl + "'>" + bn + "</a></li>"
		} else {
			breadcrumbHtml += "	    <li class='active'><a href='" + bl + "'>" + bn + "</a></li>"
		}
	}

	pathHtml := ""
	for _, path := range listing {
		pp := strings.Replace(pathPart, "$PATH_LINK", path.pathLink, 1)
		pp = strings.Replace(pp, "$PATH_ICON", path.pathIcon, 1)
		pp = strings.Replace(pp, "$PATH_NAME", path.pathName, 1)
		pp = strings.Replace(pp, "$FILE_SIZE", path.fileSize, 1)
		pathHtml += pp
	}

	shtml = strings.Replace(shtml, "$DISK_SPACE", diskSpace, 1)
	shtml = strings.Replace(shtml, "$BREADCRUMB", breadcrumbHtml, 1)
	shtml = strings.Replace(shtml, "$DIRECTORY_LISTING", pathHtml, 1)
	shtml = strings.Replace(shtml, "$UPLOAD_URL", uploadUrl, 1)
	shtml = strings.Replace(shtml, "$PUBLIC_URL", public_url, 10)

	fmt.Fprint(w, shtml)
}
