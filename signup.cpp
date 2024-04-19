#include <iostream>
#include <fstream>
#include <string>
#include <openssl/sha.h>
#include <iomanip>

using namespace std;

bool register_user(const string& username, const string& password) {
    // Open the credentials file in append mode
    ofstream outfile("credentials.txt", ios::app);
    if (!outfile)
    {
        // Failed to open the file
        cerr << "Failed to open credentials.txt file for writing." << endl;
        return false;
    }

    // Hash the password using SHA-256
    unsigned char hashed_pass[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char *)password.c_str(), password.length(), hashed_pass);

    // Convert the hashed password to a string for storage
    stringstream hashed_pass_ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        hashed_pass_ss << hex << setw(2) << setfill('0') << (int)hashed_pass[i];
    }
    string hashed_pass_str = hashed_pass_ss.str();

    // Write the new user credentials to the file with comma separation
    outfile << username << "," << hashed_pass_str << endl;
    outfile.close();

    return true;
}

int main() {
    string username, password;

    cout << "Enter a new username: ";
    getline(cin, username);

    cout << "Enter a password: ";
    getline(cin, password);

    if (register_user(username, password)) {
        cout << "Registration successful!" << endl;
    } else {
        cout << "Registration failed. Please try again later." << endl;
    }

    return 0;
}
