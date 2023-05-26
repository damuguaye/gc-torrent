package torrent

/*
#include<stdio.h>
#include<stdlib.h>
#include "./../include/torrent_struct.h"
*/
import "C"

import (
	"fmt"
	"io"
	"io/ioutil"
	"math/rand"
	"os"
	"syscall"
	"unsafe"
)

const SHALEN int = 20

type GoTorrent struct {
	Announce    string
	InfoSHA     [20]byte
	Name        string
	Length      int
	PieceLength int
	PieceSHA    [][20]byte
}
type GoPeers struct {
	Interval int
	Peers    string
	Peers6   string
}

func C2GoTorrent(up uintptr) *GoTorrent {
	ts := (*C.struct_TorrentStruct)(unsafe.Pointer(up))
	gt := new(GoTorrent)
	gt.Announce = string(C.GoBytes(unsafe.Pointer(ts.announce), ts.announce_length))
	fmt.Printf("gt.Announce: %v\n", gt.Announce)
	gt.Name = string(C.GoBytes(unsafe.Pointer(ts.name), ts.name_length))
	copy(gt.InfoSHA[:], C.GoBytes(unsafe.Pointer(ts.infoSHA), C.int(SHALEN))[:])
	gt.Length = int(int64(ts.length))
	gt.PieceLength = int(int64(ts.piece_length))

	dll := syscall.MustLoadDLL("../include/tf.dll")
	funcACA := dll.MustFindProc("addCharArray")

	piece_count := int(int64(ts.piece_count))

	addrChar := uintptr(unsafe.Pointer(ts.pieceSHA))

	gt.PieceSHA = make([][SHALEN]byte, piece_count)
	for i := 0; i < piece_count; i++ {
		copy(gt.PieceSHA[i][:], C.GoBytes(unsafe.Pointer(addrChar), C.int(SHALEN)))
		addrChar, _, _ = funcACA.Call(uintptr(unsafe.Pointer(addrChar)), uintptr(SHALEN))
	}

	return gt
}

func ParseTorrentFile(path string) *GoTorrent {
	cpath := uintptr(unsafe.Pointer(C.CString(path)))
	dll := syscall.MustLoadDLL("../include/tf.dll")
	funcGetTorFile := dll.MustFindProc("getTorrentFile")
	ts, _, _ := funcGetTorFile.Call(cpath)

	return C2GoTorrent(ts)
}

func C2GoPeers(up uintptr) *GoPeers {
	trs := (*C.struct_TrackRespStruct)(unsafe.Pointer(up))
	gp := new(GoPeers)
	gp.Interval = int(int64(trs.interval))
	gp.Peers = string(C.GoBytes(unsafe.Pointer(trs.peers), trs.peers_length))
	if int(trs.peers6_length) > 1 {
		gp.Peers6 = string(C.GoBytes(unsafe.Pointer(trs.peers6), trs.peers6_length))
		//fmt.Println(int(trs.peers6_length), "LLLLLLLLLLLLLLL")
	} else {
		gp.Peers6 = ""
		//fmt.Println(int(trs.peers6_length), "NNNNNNNNNNNNN")
	}
	return gp
}

func GetGoPeers(ir io.ReadCloser) *GoPeers {
	body, err := ioutil.ReadAll(ir)
	if err != nil {
		fmt.Println("Fail to Read Body: " + err.Error())
		return nil
	}
	tmppath := os.TempDir() + "/body-"
	var random [10]byte
	for i := range random {
		random[i] = byte('a' + (rand.Intn(1000) % 26))
	}
	tmppath = tmppath + string(random[:]) + ".txt"
	fmt.Println("tmpfile:   ", tmppath)
	file, err := os.OpenFile(tmppath, os.O_CREATE|os.O_TRUNC, 0777)
	if err != nil {
		fmt.Println("Fail create temp file: " + err.Error())
		return nil
	}

	_, err = file.Write(body)
	if err != nil {
		fmt.Println("Fail write to temp file: " + err.Error())
		return nil
	}
	file.Close()
	dll := syscall.MustLoadDLL("../include/tf.dll")
	funcGetPeers := dll.MustFindProc("getPeers")
	cpath := uintptr(unsafe.Pointer(C.CString(tmppath)))

	cPeers, _, _ := funcGetPeers.Call(cpath)
	gp := C2GoPeers(cPeers)
	os.Remove(tmppath)
	return gp
}
