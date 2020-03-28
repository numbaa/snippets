# Golang

> pre define
```go
func checkPanic(err error) { if err != nil { panic(err) }}
```

## 读取文件所有内容
```go
func main() {
	content, err := ioutil.ReadFile("main.go")
	if err != nil {
		panic(err)
	}
	fmt.Println(string(content))
}
```

## TCP服务端与客户基本模式
```go
func writeLoop(conn net.Conn)   {}
func readLoop(conn net.Conn)    {}
func onAccepted(conn net.Conn)  { fmt.Println("Accept", conn.RemoteAddr()) }
func onConnected(conn net.Conn) { fmt.Println("Connect", conn.RemoteAddr()) }
func server() {
	l, err := net.Listen("tcp", "127.0.0.1:9900")
	checkPanic(err)
	go func() {
		for {
			//记住处理EAGAIN
			conn, err := l.Accept()
			checkPanic(err)
			//conn应为自定义net.Conn的实现，否则writeLoop需要多传入一个sendChan
			go writeLoop(conn)
			go readLoop(conn)
			go onAccepted(conn)
		}
	}()
}

func client() {
	conn, err := net.Dial("tcp", "127.0.0.1:9900")
	checkPanic(err)
	go writeLoop(conn)
	go readLoop(conn)
	go onConnected(conn)
}

func main() {
	server()
	client()
	time.Sleep(time.Second * 3)
}

```

## 读写二进制协议流
```go
func main() {
	//读写二进制需要用到 io.Reader、io.Writer

	//bytes.Buffer实现了这两个接口
	buff := &bytes.Buffer{}
	var size uint32 = 666
	binary.Write(buff, binary.LittleEndian, size)
	size = 0
	binary.Read(buff, binary.LittleEndian, &size)
	fmt.Println("size:", size)

	//net.Conn包含了这两个接口
	const headerSize = 12
	type msgHeader struct {
		id       uint32
		bodySize uint32
		crc      uint32
	}
	var header msgHeader
	conn, _ := net.Dial("tcp", "127.0.0.1:8800")
	binary.Read(conn, binary.BigEndian, &header)

	//网络协议一般需要'向前看几个字节'，所以需要组合net.Conn和bytes.Buffer
	conn.SetReadDeadline(time.Now().Add(time.Millisecond * 2))
	buff.ReadFrom(conn)
	if buff.Len() < headerSize {
		//跳出控制流，如果是循环中则continue
	}
	binary.Read(buff, binary.BigEndian, &header)
}
```