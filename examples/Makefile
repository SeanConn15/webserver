tls: tls_server.c
	$(CC) `pkg-config --cflags openssl` -o $@ $^ `pkg-config --libs openssl`
	@echo "Don't forget to create cert.pem and key.pem"
