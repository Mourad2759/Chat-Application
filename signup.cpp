#include <iostream>
#include <fstream>
#include <string>
#include <openssl/sha.h>
#include <iomanip>

using namespace std;

const int TABLE_SIZE = 100; 

struct Node {
    string username;
    string hashed_pass;
    Node* next;

    Node(const string& u, const string& h) : username(u), hashed_pass(h), next(nullptr) {}
};

class HashMap {
private:
    Node* table[TABLE_SIZE];

    int hashFunction1(const string& key) {
        int hashValue = 0;
        for (char c : key) {
            hashValue += c;
        }
        return hashValue % TABLE_SIZE;
    }

    int hashFunction2(const string& key) {
        int hashValue = 0;
        for (char c : key) {
            hashValue += c * 31; // Using a different prime number as a multiplier for the second hash function
        }
        return (hashValue % 97) + 1; // Ensures the second hash value is non-zero and less than TABLE_SIZE
    }

public:
    HashMap() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            table[i] = nullptr;
        }
    }

    ~HashMap() {
    }

    void insert(const string& username, const string& hashed_pass) {
        int index = hashFunction1(username);
        if (table[index] == nullptr) {
            table[index] = new Node(username, hashed_pass);
        } else {
            // Use double hashing for collision resolution
            int index2 = hashFunction2(username);
            int step = 1;
            while (true) {
                int newIndex = (index + step * index2) % TABLE_SIZE;
                if (table[newIndex] == nullptr) {
                    table[newIndex] = new Node(username, hashed_pass);
                    break;
                }
                ++step;
            }
        }
    }

    // Function to check if a username already exists in the hashmap
    bool usernameExists(const string& username) {
        int index = hashFunction1(username);
        int index2 = hashFunction2(username);
        int step = 1;
        while (true) {
            int newIndex = (index + step * index2) % TABLE_SIZE;
            if (table[newIndex] == nullptr) {
                return false; // Username not found
            } else if (table[newIndex]->username == username) {
                return true; // Username found
            }
            ++step;
        }
    }

    // Function to save user credentials to a text file
    void saveToFile(const string& filename) {
        ofstream outfile(filename, ios::app); // Open file in append mode
        if (!outfile) {
            cerr << "Failed to open file for writing." << endl;
            return;
        }

        for (int i = 0; i < TABLE_SIZE; ++i) {
            Node* current = table[i];
            while (current != nullptr) {
                outfile << current->username << "," << current->hashed_pass << endl;
                current = current->next;
            }
        }
        outfile.close();
    }
};

// Function to hash the password using SHA-256 algorithm
string hash_password(const string& password) {
    unsigned char hashed_pass[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char *)password.c_str(), password.length(), hashed_pass);

    // Convert the hash to a string
    stringstream hashed_pass_ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hashed_pass_ss << hex << setw(2) << setfill('0') << (int)hashed_pass[i];
    }
    return hashed_pass_ss.str();
}

// Function to check if a password meets the requirements
bool is_valid_password(const string& password) {
    bool has_upper = false;
    bool has_lower = false;
    bool has_digit = false;
    bool has_special = false;

    for (char c : password) {
        if (isupper(c)) {
            has_upper = true;
        } else if (islower(c)) {
            has_lower = true;
        } else if (isdigit(c)) {
            has_digit = true;
        } else if (ispunct(c)) {
            has_special = true;
        }
    }

    return password.length() >= 8 && has_upper && has_lower && has_digit && has_special;
}

// Function to register a new user
bool register_user(HashMap& hashmap, const string& username, const string& password) {
    // Check if the password meets the requirements
    if (!is_valid_password(password)) {
        cerr << "Password does not meet the requirements." << endl;
        return false;
    }

    // Hash the password
    string hashed_pass = hash_password(password);

    // Check if the username already exists
    if (hashmap.usernameExists(username)) {
        cerr << "Username already exists." << endl;
        return false;
    }

    // Insert the username and hashed password into the hashmap
    hashmap.insert(username, hashed_pass);

    return true;
}

int main() {
    HashMap hashmap;

    string username, password;

    cout << "Enter a new username: ";
    getline(cin, username);

    // Prompt user to enter password until it meets the requirements
    while (true) {
        cout << "Enter a password: ";
        getline(cin, password);

        if (is_valid_password(password)) {
            break; // Exit the loop if password meets the requirements
        } else {
            cout << "Password must be at least 8 characters long and contain at least one uppercase letter, one lowercase letter, one digit, and one special character." << endl;
        }
    }

    if (register_user(hashmap, username, password)) {
        cout << "Registration successful!" << endl;
        
        // Save user credentials to a text file
        hashmap.saveToFile("credentials.txt");
    } else {
        cout << "Registration failed. Please try again later." << endl;
    }

    return 0;
}
