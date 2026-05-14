package main

import (
	"fmt"
	"net"
)

func main() {
	go handleServer(":9000")
	go handleServer(":9001")
	go handleServer(":9002")

	select {}
}

func handleServer(port string) {
	fmt.Println("Listening on port:", port)
	ln, _ := net.Listen("tcp", port)

	for {
		conn, err := ln.Accept()
		if err != nil {
			fmt.Println("err:", err.Error())
			continue
		}

		conn.Write([]byte("Response From Backend " + port))

		if err := conn.Close(); err != nil {
			fmt.Println("err:", err.Error())
			continue
		}
	}
}
