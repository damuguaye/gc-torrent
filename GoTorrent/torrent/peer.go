package torrent

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"strconv"
	"time"
)

type MsgId uint8

const (
	MsgChoke       MsgId = 0
	MsgUnchoke     MsgId = 1
	MsgInterested  MsgId = 2
	MsgNotInterest MsgId = 3
	MsgHave        MsgId = 4 //peer从别处下载到一个piece，所以发送一个MsgHave
	MsgBitfield    MsgId = 5
	MsgRequest     MsgId = 6
	MsgPiece       MsgId = 7
	MsgCancel      MsgId = 8
)

type PeerMsg struct {
	Id      MsgId
	Payload []byte
}

type PeerConn struct {
	net.Conn
	Choked  bool
	Field   Bitfield
	peer    PeerInfo
	peerId  [IDLEN]byte
	infoSHA [SHALEN]byte
}

func handshake(conn net.Conn, infoSHA [SHALEN]byte, peerId [IDLEN]byte) error {
	conn.SetDeadline(time.Now().Add(3 * time.Second))
	defer conn.SetDeadline(time.Time{}) //保持连接

	req := NewHandshakeMsg(infoSHA, peerId)
	_, err := WriteHandshake(conn, req)
	if err != nil {
		fmt.Println("send handshake failed")
		return err
	}

	res, err := ReadHandshake(conn)
	if err != nil {
		fmt.Println("read handshake failed")
		return err
	}

	if !bytes.Equal(res.InfoSHA[:], infoSHA[:]) {
		fmt.Println("check handshake failed")
		return fmt.Errorf("handshake infoSHA error: " + string(res.InfoSHA[:]))
	}
	return nil
}

func getBitfield(pc *PeerConn) error {
	pc.SetDeadline(time.Now().Add(5 * time.Second))
	defer pc.SetDeadline(time.Time{})

	msg, err := pc.ReadMsg()
	if err != nil {
		return nil
	}
	if msg == nil {
		return fmt.Errorf("bitfield readmsg error")
	}
	if msg.Id != MsgBitfield {
		return fmt.Errorf("expected msgbitfield, get " + strconv.Itoa(int(msg.Id)))
	}
	fmt.Println("fill bitfield : " + pc.peer.Ip.String())
	pc.Field = msg.Payload
	return nil
}

const LenBytes uint32 = 4

func (pc *PeerConn) ReadMsg() (*PeerMsg, error) {
	//读取peers发过来的信息
	//生成一个 PeerMsg
	lenBuf := make([]byte, LenBytes)
	_, err := io.ReadFull(pc, lenBuf)
	if err != nil {
		return nil, err
	}
	length := binary.BigEndian.Uint32(lenBuf) //大字端，左边为高位
	if length == 0 {
		err = fmt.Errorf("length of peerconn msg is 0")
		return nil, err
	}

	msgBuf := make([]byte, length)
	_, err = io.ReadFull(pc, msgBuf)
	if err != nil {
		return nil, err
	}

	return &PeerMsg{
		Id:      MsgId(msgBuf[0]),
		Payload: msgBuf[1:],
	}, nil
}

func (pc *PeerConn) WriteMsg(pm *PeerMsg) (int, error) {
	length := uint32(len(pm.Payload) + 1)
	buf := make([]byte, LenBytes+length)
	binary.BigEndian.PutUint32(buf[0:LenBytes], length)
	buf[LenBytes] = byte(pm.Id)
	copy(buf[LenBytes+1:], pm.Payload)
	return pc.Write(buf)
}

func CopyPieceData(index int, buf []byte, msg *PeerMsg) (int, error) {
	if msg.Id != MsgPiece {
		return 0, fmt.Errorf("expected MsgPiece (Id %d), got Id %d", MsgPiece, msg.Id)
	}
	if len(msg.Payload) < 8 {
		return 0, fmt.Errorf("payload too short. %d < 8", len(msg.Payload))
	}
	parsedIndex := int(binary.BigEndian.Uint32(msg.Payload[0:4]))
	if parsedIndex != index {
		return 0, fmt.Errorf("expected index %d, got %d", index, parsedIndex)
	}
	offset := int(binary.BigEndian.Uint32(msg.Payload[4:8]))
	if offset >= len(buf) {
		return 0, fmt.Errorf("offset too high. %d >= %d", offset, len(buf))
	}
	data := msg.Payload[8:]
	if offset+len(data) > len(buf) {
		return 0, fmt.Errorf("data too large [%d] for offset %d with length %d", len(data), offset, len(buf))
	}
	copy(buf[offset:], data)
	return len(data), nil
}

func GetHaveIndex(msg *PeerMsg) (int, error) {
	if msg.Id != MsgHave {
		return 0, fmt.Errorf("expected MsgHave (Id %d), got Id %d", MsgHave, msg.Id)
	}
	if len(msg.Payload) != 4 {
		return 0, fmt.Errorf("expected payload length 4, got length %d", len(msg.Payload))
	}
	index := int(binary.BigEndian.Uint32(msg.Payload))
	return index, nil
}

func NewRequestMsg(index, offset, length int) *PeerMsg {
	payload := make([]byte, 12)
	binary.BigEndian.PutUint32(payload[0:4], uint32(index))
	binary.BigEndian.PutUint32(payload[4:8], uint32(offset))
	binary.BigEndian.PutUint32(payload[8:12], uint32(length))
	return &PeerMsg{MsgRequest, payload}
}

func NewConn(peer PeerInfo, infoSHA [20]byte, peerId [IDLEN]byte) (*PeerConn, error) {
	// setup tcp conn
	addr := net.JoinHostPort(peer.Ip.String(), strconv.Itoa(int(peer.Port)))
	conn, err := net.DialTimeout("tcp", addr, 5*time.Second)
	if err != nil {
		fmt.Println("set tcp conn failed: " + addr)
		return nil, err
	}
	// torrent p2p handshake
	err = handshake(conn, infoSHA, peerId)
	if err != nil {
		fmt.Println("handshake failed")
		conn.Close()
		return nil, err
	}
	c := &PeerConn{
		Conn:    conn,
		Choked:  true,
		peer:    peer,
		peerId:  peerId,
		infoSHA: infoSHA,
	}
	// fill bitfield
	err = getBitfield(c)
	if err != nil {
		fmt.Println("fill bitfield failed, " + err.Error())
		return nil, err
	}
	return c, nil
}
