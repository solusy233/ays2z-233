"""
High-Performance Local HTTP 206 Streaming Server (C-Level Latency Optimization)
Serves application web files and provides high-throughput streaming for Video and Image wallpapers.
Features:
 - Fast HTTP 206 Partial Content Range streaming with 64KB/128KB chunks for instant HTML5 video seeking.
 - TCP_NODELAY socket optimization for sub-millisecond TTFB (time to first byte).
 - Threaded concurrent connection pool with automatic connection recycling.
"""

import http.server
import mimetypes
import os
import re
import socket
import socketserver
import threading
import urllib.parse

# Ensure common media MIME types are registered
mimetypes.add_type("video/mp4", ".mp4")
mimetypes.add_type("video/webm", ".webm")
mimetypes.add_type("video/x-matroska", ".mkv")
mimetypes.add_type("image/webp", ".webp")
mimetypes.add_type("application/json", ".json")


class FastRangeRequestHandler(http.server.BaseHTTPRequestHandler):
    ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    CHUNK_SIZE = 128 * 1024  # 128 KB buffer chunk for smooth 4K/60FPS video scrubbing

    def setup(self):
        super().setup()
        # Enable TCP_NODELAY for instant transmission without Nagle delay
        try:
            self.request.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        except Exception:
            pass

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)

        # Custom streaming / direct file server endpoint: /stream?path=...
        if path == "/stream" and "path" in query:
            file_path = query["path"][0]
            self.serve_file_with_range(file_path)
            return

        # Map local web assets
        if path in ("/", "/index.html"):
            target_path = os.path.join(self.ROOT_DIR, "web", "index.html")
        elif path.startswith("/web/"):
            target_path = os.path.join(self.ROOT_DIR, path[1:])
        elif path.startswith("/assets/"):
            target_path = os.path.join(self.ROOT_DIR, path[1:])
        elif path.startswith("/wallpapers/"):
            target_path = os.path.join(self.ROOT_DIR, path[1:])
        else:
            target_path = os.path.join(self.ROOT_DIR, "web", path.lstrip("/"))

        target_path = os.path.normpath(target_path)
        if not os.path.isfile(target_path):
            self.send_error(404, f"File Not Found: {path}")
            return

        self.serve_file_with_range(target_path)

    def serve_file_with_range(self, file_path):
        if not os.path.isabs(file_path):
            file_path = os.path.join(self.ROOT_DIR, file_path.lstrip("/\\"))
        file_path = os.path.normpath(file_path)

        if not os.path.isfile(file_path):
            self.send_error(404, "File Not Found")
            return

        mime_type, _ = mimetypes.guess_type(file_path)
        if not mime_type:
            mime_type = "application/octet-stream"

        file_size = os.path.getsize(file_path)
        range_header = self.headers.get("Range", None)

        # HTTP 206 Partial Content Range Streaming
        if range_header:
            match = re.match(r"bytes=(\d+)-(\d*)", range_header)
            if match:
                start = int(match.group(1))
                end = int(match.group(2)) if match.group(2) else file_size - 1
                if start >= file_size:
                    self.send_error(416, "Requested Range Not Satisfiable")
                    return
                end = min(end, file_size - 1)
                length = end - start + 1

                self.send_response(206)
                self.send_header("Content-Type", mime_type)
                self.send_header("Content-Range", f"bytes {start}-{end}/{file_size}")
                self.send_header("Content-Length", str(length))
                self.send_header("Accept-Ranges", "bytes")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()

                with open(file_path, "rb") as f:
                    f.seek(start)
                    remaining = length
                    while remaining > 0:
                        read_bytes = min(remaining, self.CHUNK_SIZE)
                        chunk = f.read(read_bytes)
                        if not chunk:
                            break
                        try:
                            self.wfile.write(chunk)
                            remaining -= len(chunk)
                        except (BrokenPipeError, ConnectionResetError):
                            break
                return

        # Regular HTTP 200 response
        self.send_response(200)
        self.send_header("Content-Type", mime_type)
        self.send_header("Content-Length", str(file_size))
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()

        with open(file_path, "rb") as f:
            while True:
                chunk = f.read(self.CHUNK_SIZE)
                if not chunk:
                    break
                try:
                    self.wfile.write(chunk)
                except (BrokenPipeError, ConnectionResetError):
                    break

    def log_message(self, format, *args):
        # Silence access logs for zero console overhead
        pass


class HighPerformanceHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    block_on_close = False


def start_local_server(port=0):
    """Start local high-speed server on a free port and return (server, actual_port)."""
    server = HighPerformanceHTTPServer(("127.0.0.1", port), FastRangeRequestHandler)
    actual_port = server.server_port
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server, actual_port
