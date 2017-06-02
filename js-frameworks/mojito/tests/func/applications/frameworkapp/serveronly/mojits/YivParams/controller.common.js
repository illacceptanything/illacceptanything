/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('YivParams', function(Y, NAME) {
    
    Y.namespace('mojito.controllers')[NAME] = {
        
        filteredBody: function(ac) {
            var testname = ac.params.getFromUrl('testname');
            var unsaferaw_name,stripped_name,notags_name,url_name, email_name;
                unsaferaw_name = ac.yiv.getData('body', 'name1','unsafe_raw'),
                stripped_name = ac.yiv.getData('body', 'name1','stripped'),
                notags_name = ac.yiv.getData('body', 'name1', 'notags');
            if(testname == 'url'){
                url_name = ac.yiv.getData('body','name1','url');
            }
            if(testname == 'email'){
                email_name = ac.yiv.getData('body', 'name1', 'email');
            }
            ac.done({
                desc: "Here's the filtered POST data!",
                unsaferaw_name: unsaferaw_name,
                stripped_name: stripped_name,
                notags_name: notags_name,
                url_name: url_name,
                email_name: email_name
            });
        },
        filteredHeader: function(ac){
            //var myrequest = ac.server.getRequest().headers;
            //console.log(JSON.stringify(myrequest));
            //var testingheader = ac.server.getRequest().headers.host;
            var name = ac.params.getFromUrl('testname');
            var testingheader = ac.http.getRequest().headers.testing;
            var unsaferaw_header =ac.yiv.getDataByRawName('headers', name,'unsafe_raw'),
                stripped_header = ac.yiv.getData('headers', name,'stripped'),
                notags_header = ac.yiv.getData('headers', name,'notags');
            ac.done({
                desc: 'Header filtered',
                testingheader:testingheader,
                unsaferaw_header:unsaferaw_header,
                stripped_header:stripped_header,
                notags_header:notags_header
            });
        },
        filteredParams: function(ac){
            var name = ac.params.getFromUrl('testname');
            var unsaferaw_name =ac.yiv.getDataByRawName('params', 'testname<br>','stripped');
            var unsaferaw_url,stripped_url,notags_url,unsaferaw_path,stripped_path;
            if (name=='url'){
                unsaferaw_url = ac.yiv.getUrl('unsafe_raw'),
                stripped_url = ac.yiv.getUrl('stripped'),
                notags_url = ac.yiv.getUrl('notags');
                unsaferaw_path = ac.yiv.getPath('unsafe_raw'),
                stripped_path = ac.yiv.getPath('stripped');
            }
            var fooVal,unsaferaw_val,stripped_val,notags_val;
                fooVal = ac.params.getFromUrl(name);
                unsaferaw_val =ac.yiv.getDataByRawName('params', name,'unsafe_raw');
                stripped_val = ac.yiv.getData('params', name,'stripped'),
                notags_val = ac.yiv.getData('params', name,'notags');
            var email_val;
            if(name=='email'){
                email_val = ac.yiv.getData('params', name, 'email');
            }
            ac.done({
                desc: 'Params filtered',
                unsaferaw_name:unsaferaw_name,
                unsaferaw_url:unsaferaw_url,
                stripped_url:stripped_url,
                notags_url:notags_url,
                unsaferaw_path:unsaferaw_path,
                stripped_path:stripped_path,
                fooVal:fooVal,
                unsaferaw_val:unsaferaw_val,
                stripped_val: stripped_val,
                notags_val:notags_val,
                email_val:email_val
            });
        },
        catchcookieyiv: function(ac) {
            var stripped_cookie = ac.yiv.getData("cookies", "name", "stripped"),
                unsaferaw_cookie = ac.yiv.getData("cookies", "name", "unsafe_raw"),
                notags_cookie = ac.yiv.getData("cookies", "name", "notags");
            ac.done({
                unsaferaw_cookie: unsaferaw_cookie,
                stripped_cookie: stripped_cookie,
                notags_cookie: notags_cookie
            });
        }

    };   
}, '0.0.1', {requires: ['mojito', 'mojito-yiv-addon', 'mojito-http-addon']});
