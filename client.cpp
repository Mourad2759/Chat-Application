#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include <mutex>
#define MAX_LEN 200
#define NUM_COLORS 6

using namespace std;

bool exit_flag = false;
thread t_send, t_recv;
int client_socket;
string def_col = "\033[0m";
string colors[] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"};

void catch_ctrl_c(int signal);
string color(int code);
void eraseText(int cnt); 
void send_message(int client_socket, int encryption_key);
void recv_message(int client_socket, int encryption_key);
void display_main_menu(int client_socket);
void authenticate_user(int client_socket);
string encrypt_message(const string& message, int key);
string decrypt_message(const string& encrypted_message, int key);

int main()
{
    // Creates a socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket: ");
        exit(-1);
    }
    // Initializes client socket
    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(10000); // Port no. of server
    client.sin_addr.s_addr = INADDR_ANY;
    //client.sin_addr.s_addr=inet_addr("127.0.0.1"); // Provide IP address of server
    bzero(&client.sin_zero, 0);

    // Connects to server
    if ((connect(client_socket, (struct sockaddr *)&client, sizeof(struct sockaddr_in))) == -1)
    {
        perror("connect: ");
        exit(-1);
    }
    signal(SIGINT, catch_ctrl_c);

    // Displays main menu
    display_main_menu(client_socket);

    int encryption_key = 5; // Encryption Key

    // Creates sends and receives threads
    thread t1(std::bind(send_message, client_socket, encryption_key));
    thread t2(std::bind(recv_message, client_socket, encryption_key));

    // Moves threads to t_send and t_recv
    t_send = move(t1);
    t_recv = move(t2);

    // Joins threads
    if (t_send.joinable())
        t_send.join();
    if (t_recv.joinable())
        t_recv.join();

    return 0;
}

// Handler for "Ctrl + C"
void catch_ctrl_c(int signal)
{
    char str[MAX_LEN] = "#exit";
    send(client_socket, str, sizeof(str), 0);
    exit_flag = true;
    t_send.detach();
    t_recv.detach();
    close(client_socket);
    exit(signal);
}

// Function to return color code
string color(int code)
{
    return colors[code % NUM_COLORS];
}

// Function to Erase text from terminal
void eraseText(int cnt)
{
    char back_space = 8;
    for (int i = 0; i < cnt; i++)
    {
        cout << back_space;
    }
}

// Function to send messages to everyone
void send_message(int client_socket, int encryption_key) {
    while (1) {
        cout << colors[1] << "You : " << def_col;
        char str[MAX_LEN];
        cin.getline(str, MAX_LEN);
        string encrypted_message = encrypt_message(str, encryption_key);
        send(client_socket, encrypted_message.c_str(), encrypted_message.size() + 1, 0);
        if (strcmp(str, "#exit") == 0) {
            exit_flag = true;
            t_recv.detach();
            close(client_socket);
            return;
        }
    }
}

// Function to receive messages
void recv_message(int client_socket, int encryption_key) {
    while (1) {
        if (exit_flag)
            return;
        char name[MAX_LEN], str[MAX_LEN];
        int color_code;
        int bytes_received = recv(client_socket, name, sizeof(name), 0);
        if (bytes_received <= 0)
            continue;
        recv(client_socket, &color_code, sizeof(color_code), 0);
        bytes_received = recv(client_socket, str, sizeof(str), 0);
        if (bytes_received <= 0)
            continue;
        string decrypted_message = decrypt_message(string(str), encryption_key); // Decrypt the received message
        eraseText(6);
        if (strcmp(name, "#NULL") != 0)
            cout << color(color_code) << name << " : " << def_col << decrypted_message << endl; // Display decrypted message
        else
            cout << color(color_code) << decrypted_message << endl; // Display decrypted message
        cout << colors[1] << "You : " << def_col;
        fflush(stdout);
    }
}

// Function to display main menu
void display_main_menu(int client_socket)
{
    int choice;
    do
    {
        cout << "===== Main Menu =====" << endl;
        cout << "1. Login" << endl;
        cout << "2. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;
        cin.ignore(); // Clear the input buffer

        switch (choice)
        {
        case 1:
            authenticate_user(client_socket);
            return; // Exit the loop after login
        case 2:
            cout << "Exiting chat." << endl;
            close(client_socket);
            exit(0); // Exit the program
        default:
            cout << "Invalid choice. Please try again." << endl;
            break;
        }
    } while (true);
}

// Function to authenticate user
void authenticate_user(int client_socket)
{
    char username[MAX_LEN];
    char password[MAX_LEN];
    // User Authentication
    cout << "\n===== User Authentication =====" << endl;
    cout << "Enter your username: ";
    cin.getline(username, MAX_LEN);
    cout << "Enter your password: ";
    cin.getline(password, MAX_LEN);

    // Send authentication data to the server
    send(client_socket, username, sizeof(username), 0);
    send(client_socket, password, sizeof(password), 0);

    // Handle authentication response
    char auth_response[MAX_LEN];
    recv(client_socket, auth_response, sizeof(auth_response), 0);
    if (strcmp(auth_response, "success") == 0)
    {
        cout << "Authentication successful! Welcome to the chat-room, " << username << endl;
    }
    else
    {
        cout << "Authentication failed. Invalid username or password." << endl;
        close(client_socket);
        exit(-1);
    }
}

// Function to encrypt message using Caesar Cipher
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

// Function to decrypt message using Caesar Cipher
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
