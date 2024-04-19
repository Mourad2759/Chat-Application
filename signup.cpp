#include <iostream>
#include <fstream>
#include <string>
#include <openssl/sha.h>
#include <iomanip>

using namespace std;

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

    if (register_user(username, password)) {
        cout << "Registration successful!" << endl;
    } else {
        cout << "Registration failed. Please try again later." << endl;
    }

    return 0;
}
