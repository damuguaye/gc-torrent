package torrent

import (
	"fmt"
	"io"
)

const (
	Reserved int = 8 //保留位
	HsMsgLen int = SHALEN + IDLEN + Reserved
)

type HandshakeMsg struct {
	//|len protocol(len=1)|protocol|reserved|infoSHA|peerID|
	PreStr  string
	InfoSHA [SHALEN]byte
	PeerId  [IDLEN]byte
}

func NewHandshakeMsg(infoSHA [SHALEN]byte, peerId [IDLEN]byte) *HandshakeMsg {
	return &HandshakeMsg{
		PreStr:  "BitTorrent protocol",
		InfoSHA: infoSHA,
		PeerId:  peerId,
	}
}

func WriteHandshake(w io.Writer, hsmsg *HandshakeMsg) (int, error) {
	buf := make([]byte, len(hsmsg.PreStr)+HsMsgLen+1)
	buf[0] = byte(len(hsmsg.PreStr))
	idx := 1
	idx += copy(buf[idx:], []byte(hsmsg.PreStr))
	idx += copy(buf[idx:], make([]byte, Reserved))
	idx += copy(buf[idx:], hsmsg.InfoSHA[:])
	idx += copy(buf[idx:], hsmsg.PeerId[:])
	return w.Write(buf)
}

func ReadHandshake(r io.Reader) (*HandshakeMsg, error) {
	lenBuf := make([]byte, 1)
	_, err := io.ReadFull(r, lenBuf)
	if err != nil {
		return nil, err
	}

	prelen := int(lenBuf[0])
	if prelen == 0 {
		err := fmt.Errorf("PreStr length cannot be 0")
		return nil, err
	}

	preStr := make([]byte, prelen+Reserved)
	_, err = io.ReadFull(r, preStr)
	if err != nil {
		return nil, err
	}

	var infoSHA [SHALEN]byte
	_, err = io.ReadFull(r, infoSHA[:])
	if err != nil {
		return nil, err
	}

	var peerId [IDLEN]byte
	_, err = io.ReadFull(r, peerId[:])
	if err != nil {
		return nil, err
	}
	return &HandshakeMsg{
		PreStr:  string(preStr[0:prelen]),
		InfoSHA: infoSHA,
		PeerId:  peerId,
	}, nil

}
