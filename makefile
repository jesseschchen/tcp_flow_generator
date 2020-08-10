CC = gcc
OPTIMIZE = -O2
CFLAGS = $(OPTIMIZE) -pthread -g



default: main.c
	@echo "building..."
	@$(CC) $(CFLAGS) -o main main.c
	@chmod +x main 
	@echo "finished building."

server: server.c
	@echo "building server..."
	@$(CC) $(CFLAGS) -o control_server server.c
	@chmod +x control_server
	@echo "finished building server."

client: client.c
	@echo "building client..."
	@$(CC) $(CFLAGS) -o control_client client.c
	@chmod +x control_client
	@echo "finished building client."

clean: 
	@echo "cleaning..."
	@rm -f *client *server
	@echo "finished cleaning."
