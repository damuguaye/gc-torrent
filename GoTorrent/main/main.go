package main

/*
#include<stdio.h>
#include<stdlib.h>
#include "./../include/torrent_struct.h"


*/
import "C"
import (
	"GoTorrent/torrent"
	"crypto/rand"
	"fmt"
	"os"
)

func main() {
	var path string
	if len(os.Args) == 1 {
		path = "../testfile/debian-iso.tt"
	} else if len(os.Args) != 3 || os.Args[1] != "-p" {
		fmt.Println("invalid args")
		return
	} else {
		path = os.Args[2]
	}

	gt := torrent.ParseTorrentFile(path)
	fmt.Printf("gt.Length: %v\n", gt.Length)
	fmt.Printf("gt.PieceLength: %v\n", gt.PieceLength)

	var peerId [torrent.IDLEN]byte
	_, _ = rand.Read(peerId[:])

	peers := torrent.FindPeers(gt, peerId)
	fmt.Println("peers number : ", len(peers))
	if len(peers) == 0 {
		fmt.Println("can not find peers")
		return
	}

	task := &torrent.TorrentTask{
		PeerId:   peerId,
		PeerList: peers,
		InfoSHA:  gt.InfoSHA,
		FileName: gt.Name,
		FileLen:  gt.Length,
		PieceLen: gt.PieceLength,
		PieceSHA: gt.PieceSHA,
	}

	torrent.Download(task)

}
