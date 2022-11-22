#!/usr/bin/env python

import http.server
import socketserver
import json
import traceback
import urllib.parse

DISPATCH_LISTEN = "0.0.0.0"
DISPATCH_PORT = 8099
DISPATCH_ROUTE_URL = "http://localhost:8099"

GAME_IP = "localhost"
GAME_PORT = 22102

GAME_RES_CUR_VERSION = 138541
DESIGN_DATA_CUR_VERSION = 138541

PLAYER_UID = 1

FEEDBACK_URL = "https://cdn.discordapp.com/attachments/441109559004889098/1043690740397641748/unknown.png"
NOTICES_URL = "https://cdn.discordapp.com/attachments/441109559004889098/1043690740397641748/unknown.png"
GUIDE_URL = "https://cdn.discordapp.com/attachments/441109559004889098/1043690740397641748/unknown.png"

GAME_RES_CUR_VERSION_BYTES = str(GAME_RES_CUR_VERSION).encode("utf8")
DESIGN_DATA_CUR_VERSION_BYTES = str(DESIGN_DATA_CUR_VERSION).encode("utf8")

class DispatchHandler(http.server.BaseHTTPRequestHandler):
	def version_string(self):
		return "broken pancake"

	def do_GET(self):
		url = urllib.parse.urlparse(self.path)
		query = urllib.parse.parse_qs(url.query)
		for k in query.keys():
			query[k] = query[k][-1]

		try:
			if url.path == "/query_region_list":
				self.do_query_region_list(url, query)
			elif url.path == "/sdk/login":
				self.do_sdk_login(url, query)
			elif url.path == "/sdk/token_login":
				self.do_sdk_token_login(url, query)
			elif url.path == "/query_cur_region":
				self.do_query_cur_region(url, query)
			elif url.path == "/game_res/cur_version.txt":
				self.send_response(200)
				self.end_headers()
				self.wfile.write(GAME_RES_CUR_VERSION_BYTES)
			elif url.path.startswith("/game_res/"):
				self.send_response(200)
				self.end_headers()
			elif url.path == "/design_data/cur_version.txt":
				self.send_response(200)
				self.end_headers()
				self.wfile.write(DESIGN_DATA_CUR_VERSION_BYTES)
			elif url.path == "/":
				self.send_response(200)
				self.end_headers()
				self.wfile.write(b"<head><style>@keyframes soggy{from{background-position:0 234px}to{background-position:375px 0}}body{background:url(\"/soggy_cat.png\") repeat;animation:soggy 2s linear 0s infinite}</style></head>\n")
			elif url.path == "/soggy_cat.png":
				self.send_response(200)
				self.send_header("Content-Type", "image/png")
				self.send_header("Content-Disposition", "inline")
				self.end_headers()
				with open("soggy_cat.png", "rb") as f:
					self.wfile.write(f.read())
			elif url.path == "/favicon.ico":
				self.send_response(404)
				self.wfile.write(b"404 Not Found\n")
				self.end_headers()
			else:
				self.send_response(400)
				self.end_headers()
				self.wfile.write(b"400 Bad Request\n")
		except Exception as ex:
			traceback.print_exception(ex)
			self.send_response(500)
			self.end_headers()
			self.wfile.write(b"500 Internal Server Error\n")

	def do_HEAD(self):
		url = urllib.parse.urlparse(self.path)
		query = urllib.parse.parse_qs(url.query)
		for k in query.keys():
			query[k] = query[k][-1]

		try:
			if url.path == "/game_res/cur_version.txt":
				self.send_response(200)
				self.send_header("Content-Length", str(len(GAME_RES_CUR_VERSION_BYTES)))
				self.end_headers()
			elif url.path.startswith("/game_res/"):
				self.send_response(200)
				self.send_header("Content-Length", "0")
				self.end_headers()
			elif url.path == "/design_data/cur_version.txt":
				self.send_response(200)
				self.send_header("Content-Length", str(len(DESIGN_DATA_CUR_VERSION_BYTES)))
				self.end_headers()
			else:
				self.send_response(400)
				self.wfile.write(b"400 Bad Request\n")
				self.end_headers()
		except Exception as ex:
			traceback.print_exc(ex)
			self.send_response(500)
			self.end_headers()
			self.wfile.write(b"500 Internal Server Error\n")

	def do_POST(self):
		self.send_response(501)
		self.end_headers()

	def do_query_region_list(self, url: urllib.parse.ParseResult, query: dict):
		self.send_response(200)
		self.send_header("Content-Type", "application/json")
		self.end_headers()

		REGION_TITLES = [
			"100% real cbt1 server",
			"Do you really think.",
			"That you will escape God's judgement for your sins."
		]

		query_region_list_rsp = {
			"retcode": 0,
			"clientCustomConfig": json.dumps({ "sdkenv": "2", "visitor": False, "devicelist": None }),
			# ^ these are ALL of keys it checks for in clientCustomConfig
			# "devicelist" seems completely unused?
			"regionList": list()
		}

		for i, region_title in enumerate(REGION_TITLES):
			query_region_list_rsp["regionList"].append({
				"dispatchUrl": f"{DISPATCH_ROUTE_URL:s}/query_cur_region",
				"name": f"region{i:d}",
				"title": region_title,
				"type": "DEV_PUBLIC"
			})

		self.wfile.write(json.dumps(query_region_list_rsp).encode("utf8"))
	
	LOGIN_JSON = {
		"retcode": 0,
		"data": {
			"uid": str(PLAYER_UID),
			"token": "token1",
			"email": "soggy:)",
		}
	}

	def do_sdk_login(self, url: urllib.parse.ParseResult, query: dict):
		self.send_response(200)
		self.send_header("Content-Type", "application/json")
		self.end_headers()
		self.wfile.write(json.dumps(self.LOGIN_JSON).encode("utf8"))

	def do_sdk_token_login(self, url: urllib.parse.ParseResult, query: dict):
		self.send_response(200)
		self.send_header("Content-Type", "application/json")
		self.end_headers()
		self.wfile.write(json.dumps(self.LOGIN_JSON).encode("utf8"))

	def do_query_cur_region(self, url: urllib.parse.ParseResult, query: dict):
		self.send_response(200)
		self.send_header("Content-Type", "application/json")
		self.end_headers()

		query_cur_region_rsp = {
			"retcode": 0,
			"regionInfo": {
				"gateserverIp": GAME_IP,
				"gateserverPort": GAME_PORT,
				"resourceUrl": f"{DISPATCH_ROUTE_URL:s}/game_res",
				"dataUrl": f"{DISPATCH_ROUTE_URL:s}/design_data",
				"feedbackUrl": FEEDBACK_URL,
				"bulletinUrl": NOTICES_URL,
				"handbookUrl": GUIDE_URL,
				"resVersion": GAME_RES_CUR_VERSION,
				"dataVersion": DESIGN_DATA_CUR_VERSION,
				# these are unused I think
				"clientCustomConfig": json.dumps({}),
				"regionCustomConfig": json.dumps({})
			}
		}

		self.wfile.write(json.dumps(query_cur_region_rsp).encode("utf8"))

httpd = None
try:
	socketserver.TCPServer.allow_reuse_address = True
	httpd = socketserver.TCPServer((DISPATCH_LISTEN, DISPATCH_PORT), DispatchHandler)
	print(f"dispatch listening on port {DISPATCH_PORT:d}")
	httpd.serve_forever()
except KeyboardInterrupt:
	print("\nKeyboardInterrupt")
finally:
	if httpd != None:
		httpd.server_close()
