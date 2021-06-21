package main

import (
	"strconv"
	"strings"
)

func reverseStringSlice(numbers []string) {
	for i, j := 0, len(numbers)-1; i < j; i, j = i+1, j-1 {
		numbers[i], numbers[j] = numbers[j], numbers[i]
	}
}

func contains(s []string, value string) bool {
	for _, v := range s {
		if v == value {
			return true
		}
	}
	return false
}

func isLocalAddress(ip string) bool {
	if strings.Compare(ip, "127.0.0.1") == 0 {
		return true
	}

	v := strings.Split(ip, ".")
	if len(v) != 4 {
		return false
	}

	a, errA := strconv.Atoi(v[0])
	b, errB := strconv.Atoi(v[1])
	if errA != nil || errB != nil {
		return false
	}

	if (a == 10) || ((a == 192) && (b == 168)) || ((a == 172) && (b >= 16) && (b <= 31)) {
		return true
	}

	return false
}
