# cst-nginx

支持dtls
patch -p1 -i patches/nginx-1.13.9-dtls-experimental.diff
## 配置文件
### 起始节点（加密节点）
```
stream {
	error_log ./logs/error.log debug;
	upstream nginx-slave {
		server 172.16.202.64:25157;
	}
	server {
		listen 25156 udp reuseport;
		proxy_pass nginx-slave;
  	proxy_ssl on;
		proxy_ssl_session_reuse off;
		proxy_timeout 50s;
		proxy_buffer_size 128k;
    proxy_ssl_verify off;
		proxy_ssl_protocols           DTLSv1.2;
		proxy_ssl_certificate         /app/cst-nginx/conf/certs/nginx.fastudpslave.com.crt;
    proxy_ssl_certificate_key     /app/cst-nginx/conf/certs/nginx.fastudpslave.com.key;
		proxy_ssl_ciphers             MEDIUM:!aNULL:!MD5;
	}
}

```
### 结束节点（解密节点）
```
stream {
  error_log ./logs/error.log debug;
  upstream slave {
    server 192.168.207.202:30002;
  }
  server {
    listen 25157 udp ssl reuseport;
    ssl_certificate /app/cst-nginx/conf/certs/nginx.fastudpslave.com.crt;
    ssl_certificate_key /app/cst-nginx/conf/certs/nginx.fastudpslave.com.key;
    ssl_session_timeout 5m;
    ssl_protocols DTLSv1.2;
    ssl_handshake_timeout 5;

    ssl_ciphers  MEDIUM:!aNULL:!MD5;
    proxy_pass slave;
    proxy_bind off;
    proxy_buffer_size 64k;
    proxy_timeout 50s;
    proxy_ssl off;
  }
}
```
## nginx编译
```
./configure --with-stream --with-stream_ssl_module --with-openssl=./dep/openssl-1.1.1l --with-debug
```

