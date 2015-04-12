package main

import "fmt"
import "time"

func main() {
	fmt.Println("Yep, this should serve content")
	fmt.Println("As long as the content you're hoping to serve is exactly these strings, and the location you're looking to serve them to is exactly standard out.")
	fmt.Println("get up")
	time.Sleep(1)
	fmt.Println("get on up")
	time.Sleep(1)
	fmt.Println("get up")
	time.Sleep(1)
	fmt.Println("get on up")
	time.Sleep(1)
	fmt.Println("and DANCE")
	time.Sleep(1)
	fmt.Println(":D--<")
	time.Sleep(1)
	fmt.Println(":D|-<")
	time.Sleep(1)
	fmt.Println(":D/-<")
	time.Sleep(1)
	// Naturaly & logically here should be a panic.
	fmt.Println("Panicking!")
	panic("Aaaaa")
}
