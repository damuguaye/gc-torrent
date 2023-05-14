package torrent

import (
	"fmt"
	"math/rand"
	"net"
	"testing"
)

func TestPeer(t *testing.T) {
	var peer PeerInfo
	peer.Ip = net.ParseIP("37.187.115.55")
	peer.Port = uint16(51413)
	path := "../testfile/debian-iso.tt"
	tf := ParseTorrentFile(path)

	var peerId [IDLEN]byte
	_, _ = rand.Read(peerId[:])

	conn, _ := NewConn(peer, tf.InfoSHA, peerId)

	fmt.Println(conn)
}
