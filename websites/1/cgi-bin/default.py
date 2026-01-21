#!/usr/bin/env python3
import os
import sys
import cgi
import cgitb
import html
import json
from urllib.parse import parse_qs

# Just for test
import time

cgitb.enable()

UPLOAD_DIR = "websites/1/files/"

def main():
    # time.sleep(10)
    method = os.environ.get("REQUEST_METHOD", "")
    action = os.environ.get("ACTION", "")
    allowed_methods = os.environ.get("METHODS_ALLOWED", "").split()
    
    if method not in allowed_methods:
        send_response(405, "Method Not Allowed", {"error": "Method not allowed"})
        return
    
    # Check upload folder exist
    if not os.path.exists(UPLOAD_DIR):
        os.makedirs(UPLOAD_DIR)
    
    if method == "GET" and action == "download":
        handle_download()
    elif method == "POST" and action == "upload":
        handle_upload()
    elif method == "DELETE" and action == "delete":
        handle_delete()
    elif method == "GET" and action == "list":
        handle_list()
    else:
        send_response(400, "Bad Request", {"error": "Unknown action or method not supported"})


# Download
def handle_download():
    try:
        query_string = os.environ.get("QUERY_STRING", "")
        params = parse_qs(query_string)
        
        if "filename" not in params:
            send_response(400, "Bad Request", {"error": "File name not specified"})
            return
        
        filename = params["filename"][0]
        
        safe_filename = os.path.basename(filename)
        filepath = os.path.join(UPLOAD_DIR, safe_filename)
        
        if not os.path.exists(filepath):
            send_response(404, "Not Found", {"error": "File not found"})
            return
        
        # Find MIME type
        content_type = "application/octet-stream"
        if filename.lower().endswith(('.jpg', '.jpeg')):
            content_type = "image/jpeg"
        elif filename.lower().endswith('.png'):
            content_type = "image/png"
        elif filename.lower().endswith('.pdf'):
            content_type = "application/pdf"
        elif filename.lower().endswith('.txt'):
            content_type = "text/plain"
        elif filename.lower().endswith('.html'):
            content_type = "text/html"
        
        with open(filepath, 'rb') as f:
            file_content = f.read()
     
        print("HTTP/1.1 200 OK")
        print(f"Content-Type: {content_type}")
        print(f"Content-Length: {len(file_content)}")
        print(f"Content-Disposition: attachment; filename=\"{safe_filename}\"")
        print("Access-Control-Allow-Origin: *")
        print()
        sys.stdout.flush()
      

        sys.stdout.buffer.write(file_content)
        sys.stdout.flush()
        
    except Exception as e:
        send_response(500, "Internal Server Error", {"error": str(e)})


# Upload
def handle_upload():
    try:
        form = cgi.FieldStorage()

        if "file" not in form:
            send_response(400, "Bad Request", {"error": "No file found"})
            return
        
        fileitem = form["file"]

        if not fileitem.filename:
            send_response(400, "Bad Request", {"error": "Empty file"})
            return
        
        filename = os.path.basename(fileitem.filename)
        filepath = os.path.join(UPLOAD_DIR, filename)

        with open(filepath, 'wb') as f:
            data = fileitem.file.read()
            f.write(data)

        actual_size = os.path.getsize(filepath)

        send_response(201, "Created", {
            "success": True,
            "message": "File uploaded successfully",
            "filename": filename,
            "path": filepath
        })

    except Exception as e:
        send_response(500, "Internal Server Error", {"error": str(e)})


# Delete
def handle_delete():
    try:
        query = os.environ.get("QUERY_STRING", "")
        if not query:
            send_response(400, "Bad request", {"error": "Query string is empty"})
            return

        params = parse_qs(query)
        filename = params.get("filename", [""])[0]
        if not filename:
            send_response(400, "Bad request", {"error": "No filename"})
            return

        safe_filename = os.path.basename(filename)
        filepath = os.path.join(UPLOAD_DIR, safe_filename)

        if not os.path.exists(filepath):
            send_response(404, "Not Found", {"error": filepath})
            return

        os.remove(filepath)

        send_response(200, "OK", {
            "success": True,
            "message": "File deleted successfully",
            "filename": safe_filename
        })

    except Exception as e:
        send_response(500, "Internal Server Error", {"error": str(e)})


# List files
def handle_list():
    try:
        files = []
        if os.path.exists(UPLOAD_DIR):
            for filename in os.listdir(UPLOAD_DIR):
                filepath = os.path.join(UPLOAD_DIR, filename)
                if os.path.isfile(filepath):
                    files.append({
                        "name": filename,
                        "size": os.path.getsize(filepath),
                        "modified": os.path.getmtime(filepath)
                    })
        
        send_response(200, "OK", {
            "success": True,
            "files": files
        })
    except Exception as e:
        send_response(500, "Internal Server Error", {"error": str(e)})



def send_response(status_code, status_message, data):
    print(f"HTTP/1.1 {status_code} {status_message}")
    print("Content-Type: application/json")
    print("Connection: keep-alive")
    
    json_data = json.dumps(data)
    print(f"Content-Length: {len(json_data)}")
    print()
    
    print(json_data)



if __name__ == "__main__":
    main()
