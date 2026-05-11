package main

import (
	"fmt"
	"net"
)

func main() {
	ln, _ := net.Listen("tcp", ":9000")

	for {
		conn, err := ln.Accept()
		if err != nil {
			fmt.Println("err:", err.Error())
			continue
		}

		conn.Write([]byte("Response From Backend"))

		if err := conn.Close(); err != nil {
			fmt.Println("err:", err.Error())
			continue
		}
	}
}
