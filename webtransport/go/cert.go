package main

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"crypto/sha256"
	"crypto/tls"
	"crypto/x509"
	"crypto/x509/pkix"
	_ "embed"
	b64 "encoding/base64"
	"encoding/binary"
	"encoding/pem"
	"fmt"
	"log"
	"math/big"
	"net/http"
	"strings"
	"time"
)

func GenerateCertAndStartServer(port int) ([]byte, []byte) {
	tlsConf, x509AsBytes, err := getTLSConf(time.Now(), time.Now().Add(10*24*time.Hour))
	if err != nil {
		log.Fatal(err)
	}
	cert := tlsConf.Certificates[0]
	hash := sha256.Sum256(cert.Leaf.Raw)
	go runHTTPServer(port, hash)

	derBuf, _ := x509.MarshalECPrivateKey(cert.PrivateKey.(*ecdsa.PrivateKey))

	pem1 := pem.EncodeToMemory(&pem.Block{
		Type:  "CERTIFICATE",
		Bytes: x509AsBytes,
	})

	priv := pem.EncodeToMemory(&pem.Block{
		Type:  "PRIVATE KEY",
		Bytes: derBuf,
	})

	return pem1, priv
}

func runHTTPServer(port int, certHash [32]byte) {
	mux := http.NewServeMux()
	mux.HandleFunc("/hash", func(w http.ResponseWriter, _ *http.Request) {
		w.Write([]byte(b64.StdEncoding.EncodeToString(certHash[:])))
	})
	http.ListenAndServe(fmt.Sprintf(":%d", port), mux)
}

func getTLSConf(start, end time.Time) (*tls.Config, []byte, error) {
	cert, bytes, priv, err := generateCert(start, end)
	if err != nil {
		return nil, nil, err
	}

	return &tls.Config{
		Certificates: []tls.Certificate{{
			Certificate: [][]byte{cert.Raw},
			PrivateKey:  priv,
			Leaf:        cert,
		}},
	}, bytes, nil
}

func generateCert(start, end time.Time) (*x509.Certificate, []byte, *ecdsa.PrivateKey, error) {
	b := make([]byte, 8)
	if _, err := rand.Read(b); err != nil {
		return nil, nil, nil, err
	}
	serial := int64(binary.BigEndian.Uint64(b))
	if serial < 0 {
		serial = -serial
	}
	certTempl := &x509.Certificate{
		SerialNumber:          big.NewInt(serial),
		Subject:               pkix.Name{},
		NotBefore:             start,
		NotAfter:              end,
		IsCA:                  true,
		ExtKeyUsage:           []x509.ExtKeyUsage{x509.ExtKeyUsageClientAuth, x509.ExtKeyUsageServerAuth},
		KeyUsage:              x509.KeyUsageDigitalSignature | x509.KeyUsageCertSign,
		BasicConstraintsValid: true,
	}
	caPrivateKey, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		return nil, nil, nil, err
	}
	caBytes, err := x509.CreateCertificate(rand.Reader, certTempl, certTempl, &caPrivateKey.PublicKey, caPrivateKey)
	if err != nil {
		return nil, nil, nil, err
	}
	ca, err := x509.ParseCertificate(caBytes)
	if err != nil {
		return nil, nil, nil, err
	}
	return ca, caBytes, caPrivateKey, nil
}

func formatByteSlice(b []byte) string {
	s := strings.ReplaceAll(fmt.Sprintf("%#v", b[:]), "[]byte{", "[")
	s = strings.ReplaceAll(s, "}", "]")
	return s
}
