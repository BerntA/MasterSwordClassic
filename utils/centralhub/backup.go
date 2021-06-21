package main

import (
	"fmt"
	"strings"
	"time"
)

func runScheduledBackup() {
	var bldr strings.Builder
	for {
		bldr.Reset()
		// create backup here...
		fmt.Println("Wrote backup:", time.Now().String())
		time.Sleep(4 * time.Hour)
	}
}
