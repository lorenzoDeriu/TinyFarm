import sys
import struct
import socket
import threading

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 65001
LONG_SIZE = 8
STRING_SIZE = 255
END_OF_TASK = "__END___OF____TASK__"


class ClientThread(threading.Thread):
	def __init__(self, connection, address, instruction):
		threading.Thread.__init__(self)
		self.conn = connection
		self.addr = address
		self.instruction = instruction
	
	def run(self):
		# print("====", self.ident, "mi occupo di", self.addr)
		connection_handling(self.conn, self.addr, self.instruction)
		# print("====", self.ident, "ho finito")



class Instruction:
	def __init__(self):
		self.finished = False

	def shutdown(self):
		self.finished = True
		print("segnale di terminazione mandato")


def main(host=DEFAULT_HOST, port=DEFAULT_PORT):
	instruction = Instruction()

	with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
		try:
			server.bind((host, port))
			server.listen()

			while not instruction.finished:
				# print("Processo Collector in attesa di richieste...", instruction.finished)

				if instruction.finished: break

				connection, address = server.accept()
				
				thread = ClientThread(connection, address, instruction)
				thread.start()
		except KeyboardInterrupt: pass
	
		print("Server Chiuso")
		server.shutdown(socket.SHUT_RDWR)



def connection_handling(connection, address, instruction):
	with connection:
		data = connection.recv(4)
		length_result = struct.unpack("!i", data[:4])[0]

		data = connection.recv(length_result)
		result = data[:length_result]

		data = connection.recv(4)
		length_file_name = struct.unpack("!i", data[:4])[0]

		data = connection.recv(length_file_name)
		file_name = data[:length_file_name]


		if file_name.decode() == END_OF_TASK: 
			instruction.shutdown() # TODO trovare soluzione migliore
			return

		print(f"{result.decode()} {file_name.decode()}")



# def recv_all(conn,n):
# 	chunks = b''
# 	bytes_recd = 0
# 	while bytes_recd < n:
# 		chunk = conn.recv(min(n - bytes_recd, 1024))
# 		if len(chunk) == 0:
# 			raise RuntimeError("socket connection broken")
# 		chunks += chunk
# 		bytes_recd = bytes_recd + len(chunk)
# 	return chunks



if len(sys.argv) == 1:
	main()
elif len(sys.argv) == 2:
	main(sys.argv[1])
elif len(sys.argv) == 3:
	main(sys.argv[1], sys.argv[2])
else:
	print(f"usage {sys.argv[0]} [host] [port]")

