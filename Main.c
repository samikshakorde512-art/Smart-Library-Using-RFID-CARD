#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
// RFID Setup
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);
// LCD Setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad Setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, A0};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
// Buzzer Setup
#define BUZZER_PIN A1

// Book Info
const int totalBooks = 5;
String validBooks[totalBooks] = {"Ardino Book", "ESP32 Book", "EDC Book", "EMI Book", "PLC Book"};
String availableBooks[totalBooks];
int availableCount = totalBooks;

// User Info
struct User {
  String uid;
  String name;
  String issuedBooks[5];
  int issuedCount;
};

User users[] = {
  {"6550c901", "Srushti", {}, 0},
  {"76973903", "Namrata", {}, 0}
};
int totalUsers = sizeof(users) / sizeof(users[0]);

void setup() {
  Wire.begin();
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  delay(1000);

  lcd.init();
  lcd.backlight();
  pinMode(BUZZER_PIN, OUTPUT);

  // 🔔 Welcome Message
  lcd.setCursor(0, 0);
  lcd.print(" Welcome to the ");
  lcd.setCursor(0, 1);
  lcd.print("Smart Library");
  buzz();
  delay(2500);
  lcd.clear();
for (int i = 0; i < totalBooks; i++) {
    availableBooks[i] = validBooks[i];
  }

  Serial.println("System Ready. Scan your RFID card...");
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print("Scan RFID Card ");
  lcd.setCursor(0, 1);
  lcd.print("                ");

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = getUID();
  Serial.print("Card detected with UID: ");
  Serial.println(uid);
  int userIndex = getUserIndex(uid);
  lcd.clear();

  if (userIndex == -1) {
    lcd.print("Access Denied");
    buzz();
    delay(2000);
  } else {
    lcd.print("Welcome ");
    lcd.setCursor(0, 1);
    lcd.print(users[userIndex].name);
    buzz();
    delay(1500);
    handleMenu(userIndex);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(1500);
}

String getUID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toLowerCase();
  return uid;
}

void buzz() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
}

int getUserIndex(String uid) {
  for (int i = 0; i < totalUsers; i++) {
    if (users[i].uid == uid) return i;
  }
  return -1;
}

bool isBookAvailable(String bookID) {
  for (int i = 0; i < availableCount; i++) {
    if (availableBooks[i] == bookID) return true;
  }
  return false;
}

bool isValidBook(String bookID) {
  for (int i = 0; i < totalBooks; i++) {
    if (validBooks[i] == bookID) return true;
  }
  return false;
}

void removeBookFromAvailable(String bookID) {
  for (int i = 0; i < availableCount; i++) {
    if (availableBooks[i] == bookID) {
      for (int j = i; j < availableCount - 1; j++) {
        availableBooks[j] = availableBooks[j + 1];
      }
      availableCount--;
      break;
    }
  }
}

void addBookToAvailable(String bookID) {
  if (availableCount < totalBooks) {
    availableBooks[availableCount++] = bookID;
  }
}

bool hasIssuedBook(int userIndex, String bookID) {
  for (int i = 0; i < users[userIndex].issuedCount; i++) {
    if (users[userIndex].issuedBooks[i] == bookID) return true;
  }
  return false;
}

void removeIssuedBook(int userIndex, String bookID) {
  for (int i = 0; i < users[userIndex].issuedCount; i++) {
    if (users[userIndex].issuedBooks[i] == bookID) {
      for (int j = i; j < users[userIndex].issuedCount - 1; j++) {
        users[userIndex].issuedBooks[j] = users[userIndex].issuedBooks[j + 1];
      }
      users[userIndex].issuedCount--;
      break;
    }
  }
}

void handleMenu(int userIndex) {
  lcd.clear();
  lcd.print("1:Issue 2:Return");

  char opt = waitForKey();
  if (opt == '1') {
    issueBook(userIndex);
  } else if (opt == '2') {
    returnBook(userIndex);
  } else {
    lcd.clear();
    lcd.print("Invalid Option");
    delay(2000);
  }
}

void issueBook(int userIndex) {
  if (users[userIndex].issuedCount >= 5) {
    lcd.clear();
    lcd.print("Max 5 books!");
    delay(2000);
    return;
  }

  if (availableCount == 0) {
    lcd.clear();
    lcd.print("No books avail");
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.print("Books Available");
  delay(2000);

  int shown = 0;
  int mapping[availableCount];

  for (int i = 0; i < availableCount; i++) {
    lcd.clear();
    lcd.print(String(i + 1) + ") " + availableBooks[i]);
    mapping[i] = i;
    delay(1500);
    shown++;
  }

  lcd.clear();
  lcd.print("Choose 1-" + String(shown));
  char key = waitForKey();
  int index = key - '1';

  if (index < 0 || index >= shown) {
    lcd.clear();
    lcd.print("Invalid option");
    delay(2000);
    return;
  }

  String bookID = availableBooks[mapping[index]];

  if (hasIssuedBook(userIndex, bookID)) {
    lcd.clear();
    lcd.print("Already Issued");
    delay(2000);
    return;
  }

  users[userIndex].issuedBooks[users[userIndex].issuedCount++] = bookID;
  removeBookFromAvailable(bookID);

  lcd.clear();
  lcd.print("Book Issued");
  lcd.setCursor(0, 1);
  lcd.print(bookID);
  buzz();
  delay(2000);
}

void returnBook(int userIndex) {
  if (users[userIndex].issuedCount == 0) {
    lcd.clear();
    lcd.print("No books to");
    lcd.setCursor(0, 1);
    lcd.print("return.");
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.print("Your Books:");
  delay(1500);

  for (int i = 0; i < users[userIndex].issuedCount; i++) {
    lcd.clear();
    lcd.print(String(i + 1) + ") " + users[userIndex].issuedBooks[i]);
    delay(1500);
  }

  lcd.clear();
  lcd.print("Select 1-" + String(users[userIndex].issuedCount));
  char key = waitForKey();
  int index = key - '1';

  if (index < 0 || index >= users[userIndex].issuedCount) {
    lcd.clear();
    lcd.print("Invalid Choice");
    delay(2000);
    return;
  }

  String bookID = users[userIndex].issuedBooks[index];

  removeIssuedBook(userIndex, bookID);
  addBookToAvailable(bookID);

  lcd.clear();
  lcd.print("Returned by:");
  lcd.setCursor(0, 1);
  lcd.print(users[userIndex].name);
  buzz();
  delay(2000);

  lcd.clear();
  lcd.print("Thank you!");
  delay(2000);
}

char waitForKey() {
  char key = 0;
  while (!key) {
    key = keypad.getKey();
  }
  return key;
}

String readInputFromKeypad() {
  String input = "";
  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break;
      input += key;
      lcd.print(key);
    }
  }
  return input;
}

















