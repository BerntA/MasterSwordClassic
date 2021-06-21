package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"os"
	"strings"
)

var database_name = flag.String("db", "database", "Set the database to use.")
var WhitelistedAddresses *[]string

func loadJsonFile(path string, output interface{}) bool {
	data, err := ioutil.ReadFile(path)
	if err != nil {
		return false
	}

	err = json.Unmarshal(data, output)
	if err != nil {
		return false
	}

	return true
}

func defaultHandler(w http.ResponseWriter, r *http.Request) {
	fmt.Fprintf(w, "Welcome to the MSC Central Hub, please notify XYZ if you want access!\n")
}

func whitelistedHandler(w http.ResponseWriter, r *http.Request) {
	var bldr strings.Builder
	bldr.WriteString("MSC Central Hub\n\nWhitelisted:\n")
	for _, v := range *WhitelistedAddresses {
		bldr.WriteString(fmt.Sprintf("%v\n", v))
	}
	bldr.WriteString("\n")
	fmt.Fprintf(w, bldr.String())
}

func requestHandler(w http.ResponseWriter, r *http.Request) {
	ip, _, err := net.SplitHostPort(r.RemoteAddr)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	if !isLocalAddress(ip) && !contains(*WhitelistedAddresses, ip) {
		fmt.Fprintf(w, fmt.Sprintf("Your address (%v) is not whitelisted!\n", ip))
		return
	}

	fmt.Fprintf(w, "Success!\n")
}

func main() {
	flag.Parse()
	fmt.Println("Starting Central Hub...")
	go runScheduledBackup()

	fmt.Println("Loading Whitelisted Addresses...")
	var whitelisted_addresses []string
	loadJsonFile("./whitelisted.json", &whitelisted_addresses)
	WhitelistedAddresses = &whitelisted_addresses

	http.HandleFunc("/", defaultHandler)
	http.HandleFunc("/whitelisted/", whitelistedHandler)
	http.HandleFunc("/request/", requestHandler)

	go func() {
		fmt.Println("Activated RESTFul API...")
		log.Fatal(http.ListenAndServe(":8080", nil))
	}()

	scan := bufio.NewScanner(os.Stdin)
	for scan.Scan() {
		text := strings.ToLower(scan.Text())
		if strings.HasPrefix(text, "quit") {
			break
		}
	}

	fmt.Println("Shutting down Central Hub...")
	fmt.Println("")
}
