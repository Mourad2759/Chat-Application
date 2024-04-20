#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <fstream>
#include <sstream>
#include <openssl/sha.h>
#define MAX_LEN 200
#define NUM_COLORS 6

using namespace std;

struct terminal
{
    int id;
    string name;
    int socket;
    thread th;
};

vector<terminal> clients;
string def_col = "\033[0m";
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};
int seed = 0;
mutex cout_mtx, clients_mtx;

string color(int code);
void set_name(int id, const char *name);
void shared_print(string str, bool endLine);
void broadcast_message(string message, int sender_id);
void broadcast_message(int num, int sender_id);
void end_connection(int id);
void handle_client(int client_socket, int id);
string authenticate_user(int client_socket);
string encrypt_message(const string& message, int key);
string decrypt_message(const string& encrypted_message, int key);

int main()
{
    int server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket: ");
        exit(-1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(10000);
    server.sin_addr.s_addr = INADDR_ANY;
    bzero(&server.sin_zero, 0);

    if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1)
    {
        perror("bind error: ");
        exit(-1);
    }

    if ((listen(server_socket, 8)) == -1)
    {
        perror("listen error: ");
        exit(-1);
    }

    struct sockaddr_in client;
    int client_socket;
    unsigned int len = sizeof(sockaddr_in);

    cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======   " << endl
         << def_col;

    while (1)
    {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1)
        {
            perror("accept error: ");
            exit(-1);
        }
        seed++;
        thread t(handle_client, client_socket, seed);
        lock_guard<mutex> guard(clients_mtx);
        clients.push_back({seed, string("Anonymous"), client_socket, (move(t))});
    }

    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].th.joinable())
            clients[i].th.join();
    }

    close(server_socket);
    return 0;
}

string color(int code)
{
    return colors[code % NUM_COLORS];
}

// Set name of client
void set_name(int id, const char *name)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id == id)
        {
            clients[i].name = string(name);
        }
    }
}


// For synchronisation of cout statements
void shared_print(string str, bool endLine = true)
{
    lock_guard<mutex> guard(cout_mtx);
    cout << str;
    if (endLine)
        cout << endl;
}

// Broadcast message to all clients except the sender
void broadcast_message(string message, int sender_id)
{
    char encrypted_message[MAX_LEN];
    strcpy(encrypted_message, encrypt_message(message, 5).c_str());
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id != sender_id)
        {
            send(clients[i].socket, encrypted_message, sizeof(encrypted_message), 0);
        }
    }
}

// Broadcast a number to all clients except the sender
void broadcast_message(int num, int sender_id)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id != sender_id)
        {
            send(clients[i].socket, &num, sizeof(num), 0);
        }
    }
}

void end_connection(int id)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id == id)
        {
            lock_guard<mutex> guard(clients_mtx);
            clients[i].th.detach();
            clients.erase(clients.begin() + i);
            close(clients[i].socket);
            break;
        }
    }
}

void handle_client(int client_socket, int id)
{
    // Authenticate the user
    string authenticated_username = authenticate_user(client_socket);

    // If authentication fails, close the connection
    if (authenticated_username.empty())
    {
        end_connection(id);
        return;
    }

    char str[MAX_LEN];
    // Display welcome message with authenticated username
    string welcome_message = authenticated_username + " has joined";
    broadcast_message("#NULL", id);
    broadcast_message(id, id);
    broadcast_message(welcome_message, id);
    shared_print(color(id) + welcome_message + def_col);

    while (1)
    {
        int bytes_received = recv(client_socket, str, sizeof(str), 0);
        if (bytes_received <= 0)
            return;
        if (strcmp(str, "#exit") == 0)
        {
            // Display leaving message with plaintext username
            string message = authenticated_username + " has left";
            broadcast_message("#NULL", id);
            broadcast_message(id, id);
            broadcast_message(message, id);
            shared_print(color(id) + message + def_col);
            end_connection(id);
            return;
        }
        // Decrypt the received message
        string decrypted_message = decrypt_message(str, 5);
        broadcast_message(decrypt_message(authenticated_username, 5), id); // Decrypt and broadcast username
        broadcast_message(id, id);
        broadcast_message(decrypted_message, id); // Broadcast decrypted message
        shared_print(color(id) + authenticated_username + " : " + def_col + decrypted_message);
    }
}

string authenticate_user(int client_socket)
{
    char username[MAX_LEN], password[MAX_LEN];
    recv(client_socket, username, sizeof(username), 0);
    recv(client_socket, password, sizeof(password), 0);

    string user(username);
    string pass(password);

    // Open the credentials file for reading
    ifstream infile("credentials.txt");
    if (!infile)
    {
        // Failed to open the file
        send(client_socket, "failure", sizeof("failure"), 0);
        return "";
    }

    string line;
    string authenticated_username = "";

    while (getline(infile, line))
    {
        stringstream ss(line);
        string stored_username, stored_password;
        getline(ss, stored_username, ','); // Read until the comma
        getline(ss, stored_password);      // Read the rest of the line

        // Compare the stored username with the provided username
        if (stored_username == user)
        {
            // Hash the provided password using SHA-256
            unsigned char hashed_pass[SHA256_DIGEST_LENGTH];
            SHA256((const unsigned char *)pass.c_str(), pass.length(), hashed_pass);

            // Convert the hashed password to a string for comparison
            stringstream hashed_pass_ss;
            for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
            {
                hashed_pass_ss << hex << setw(2) << setfill('0') << (int)hashed_pass[i];
            }
            string hashed_pass_str = hashed_pass_ss.str();

            // Compare the hashed password with the stored password
            if (stored_password == hashed_pass_str)
            {
                // User authentication successful
                send(client_socket, "success", sizeof("success"), 0);
                authenticated_username = user;
                break; // Break out of the loop once authentication succeeds
            }
        }
    }

    infile.close();

    if (authenticated_username.empty())
    {
        // No matching credentials found
        send(client_socket, "failure", sizeof("failure"), 0);
    }

    return authenticated_username;
}

string encrypt_message(const string& message, int key) {
    string encrypted_message = message;
    for (char& c : encrypted_message) {
        if (isalpha(c)) {
            char base = isupper(c) ? 'A' : 'a';
            c = (c - base + key) % 26 + base;
        }
    }
    return encrypted_message;
}

string decrypt_message(const string& encrypted_message, int key) {
    string decrypted_message = encrypted_message;
    for (char& c : decrypted_message) {
        if (isalpha(c)) {
            char base = isupper(c) ? 'A' : 'a';
            c = (c - base - key + 26) % 26 + base;
        }
    }
    return decrypted_message;
}

