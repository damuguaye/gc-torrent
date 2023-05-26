package torrent

import (
	"encoding/binary"
	"fmt"
	"net"
	"net/http"
	"net/url"
	"strconv"
	"strings"
	"time"
)

const (
	PeerPort int = 6666
	IpLen    int = 4
	PortLen  int = 2
	PeerLen  int = IpLen + PortLen
)

const IDLEN int = 20

type PeerInfo struct {
	Ip   net.IP
	Port uint16
}

type TrackerResponse struct {
	Interval int    `bencode:"interval"`
	Peers    string `bencode:"peers"` //紧凑排列的ip，port
}

func CreateUrl(gt *GoTorrent, peerId [20]byte) (string, error) {
	base, err := url.Parse(gt.Announce)
	if err != nil {
		fmt.Println("Announce Error: " + gt.Announce)
		return "", err
	}

	params := url.Values{
		"info_hash":  []string{string(gt.InfoSHA[:])},
		"peer_id":    []string{string(peerId[:])},
		"port":       []string{strconv.Itoa(PeerPort)},
		"uploaded":   []string{"0"},
		"downloaded": []string{"0"},
		"compact":    []string{"1"},
		"left":       []string{strconv.Itoa(gt.Length)}, //剩余大小
	}

	base.RawQuery = params.Encode()
	return base.String(), nil

}

func buildPeerInfo(peers []byte) []PeerInfo {
	peer_num := len(peers) / PeerLen
	if len(peers)%PeerLen != 0 {
		fmt.Println("Received peers error")
		return nil
	}
	infos := make([]PeerInfo, peer_num)
	for i := 0; i < peer_num; i++ {
		offset := i * PeerLen
		infos[i].Ip = net.IP(peers[offset : offset+IpLen])
		infos[i].Port = binary.BigEndian.Uint16(peers[offset+IpLen : offset+PeerLen])
	}
	return infos
}

func buildPeerInfov6(peers string) []PeerInfo {
	ipPort := strings.Split(peers, "*")
	infos := make([]PeerInfo, 0)
	for _, item := range ipPort {
		if len(item) > 0 {
			var Peer PeerInfo
			fmt.Println("item: ", item)
			splitPeer := strings.Split(item, "&")
			Peer.Ip = net.ParseIP(splitPeer[0])
			portVal, _ := strconv.Atoi(splitPeer[1])
			Peer.Port = uint16(portVal)
			infos = append(infos, Peer)
		}

	}
	return infos

}

func FindPeers(gt *GoTorrent, peerId [IDLEN]byte) []PeerInfo {
	url, err := CreateUrl(gt, peerId)
	if err != nil {
		fmt.Println("Build Tracker Url Error: " + err.Error())
		return nil
	}

	cli := &http.Client{Timeout: 15 * time.Second}
	res, err := cli.Get(url)
	if err != nil {
		fmt.Println("Fail to Connect to Tracker: " + err.Error())
		return nil
	}

	defer res.Body.Close()

	gp := GetGoPeers(res.Body)
	var peers []PeerInfo
	var peers6 []PeerInfo
	if gp.Peers[0] == '*' {
		peers = buildPeerInfov6(gp.Peers)
		if len(peers6) > 0 {
			peers6 = buildPeerInfov6(gp.Peers6)
		}
	} else {
		peers = buildPeerInfo([]byte(gp.Peers))
		if len(peers6) > 0 {
			peers6 = buildPeerInfo([]byte(gp.Peers6))
		}
	}

	var i int
	var j int
	for i = range peers {
		if peers[i].Port == 0 {
			break
		}
	}
	for j = range peers6 {
		if peers6[j].Port == 0 {
			break
		}
	}

	return append(peers[0:i], peers6[0:j]...)
}
