from http.server import HTTPServer, SimpleHTTPRequestHandler
import ssl

class CORSHTTPRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        SimpleHTTPRequestHandler.end_headers(self)

if __name__ == '__main__':
    httpd = HTTPServer(('localhost', 8000), CORSHTTPRequestHandler)
    httpd.serve_forever()
