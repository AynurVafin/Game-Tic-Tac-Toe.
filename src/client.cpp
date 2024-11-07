#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <thread>

using namespace std;
// вывод ошибок
int error(const char *msg) {
  perror(msg);
  return 1;
}

// соединения клиента
int socketClient;
int Client_Number;
const int size_board = 3;  // универсальный размер поля

// пакеты передачи
enum Packet {
  P_Message,
  P_PlayGame,
  P_ExitProgramm,
  P_ExitToMenu,
  P_Menu,
  P_Check,
  P_ExitGame
};

// проверка на ввод числа от .. до ..
int get_variant(int min_number, int max_number) {
  int variant;
  string str;
  cout << "Please select from " << min_number << " to " << max_number
       << " and press enter\n";
  getline(cin, str);
  while (sscanf(str.c_str(), "%d", &variant) != 1 || variant < min_number ||
         variant > max_number) {
    cout << "Incorrect number input !!!\n";
    cout << "Please select from " << min_number << " to " << max_number
         << " and press enter\n";
    getline(cin, str);
  }
  return variant;
}

// вырисовка заголовка
void draw_header() {
  printf("\e[1;1H\e[2J");
  cout << "\n";
  char header[6][55] = {
      "\t _____ _       _____            _____",
      "\t|_   _(_)     |_   _|          |_   _|",
      "\t  | |  _  ___   | | __ _  ___    | | ___   ___",
      "\t  | | | |/ __|  | |/ _` |/ __|   | |/ _ \\ / _ \\",
      "\t  | | | | (__   | | ( | | (__    | | ( ) |  __/",
      "\t  \\_/ |_|\\___|  \\_/\\__,_|\\___|   \\_/\\___/ \\___|"};
  for (int i = 0; i < 6; i++) {
    cout << header[i] << endl;
  }
  cout << endl;
}

// вырисовка меню
void draw_menu() {
  draw_header();
  string menu[] = {"MENU", "1. Play Game", "2. Chat Message", "0. Exit"};
  int len_line_menu = 20;
  string line(len_line_menu, '-');
  for (int i = 1, k = 0; i <= 7; i++) {
    if (i == 1 || i == 3 || i == 7)
      cout << '+' << line << '+' << endl;
    else {
      cout << "|  " << menu[k] << setw(21 - menu[k].size() - 2) << '|' << endl;
      ++k;
    }
  }
}

// вырисовка игрового поля
void draw_game(vector<vector<char> > &board, int check_progress) {
  draw_header();
  cout << "\nThe board currently is:" << endl;
  int len_board = 23;
  for (int i = 0; i < size_board; i++) {
    string line(len_board, '-');
    cout << '+' << line << '+' << endl;

    for (int j = 0; j < size_board; j++) {
      cout << "|   " << board[i][j] << "   ";
      if (j == 2) cout << "|";
    }
    cout << endl;
    if (i == 2) {
      string line(len_board, '-');
      cout << '+' << line << '+' << endl;
    }
  }
  if (check_progress > 0) {
    cout << "Enter your turn: ";
    while (true) {
      int variant = get_variant(1, 9);
      int checker;

      Packet packettype = P_Check;
      send(socketClient, (char *)&packettype, sizeof(Packet), 0);
      send(socketClient, (char *)&variant, sizeof(int), 0);
      recv(socketClient, (char *)&checker, sizeof(int), 0);
      if (checker) break;
      cout << "This cell is occupied!" << endl;  // занята ячейка
    }
  } else if (check_progress == 0)
    cout << "Please wait for the other player's turn..." << endl;
  else if (check_progress == -1)
    cout << "Congratulations! You won!\n" << endl;
  else if (check_progress == -2)
    cout << "Sorry! You loose!\n" << endl;
  else
    cout << "You have a draw!\n" << endl;
}

// отправка сообщений чата
void chat_message() {
  draw_header();
  cout << "!!! Please write only \"/exit\" to exit the chat !!!\n\n";
  string msg_chat = "--Join Chat--";
  int msg_size = msg_chat.size();
  Packet packettype = P_Message;
  send(socketClient, (char *)&packettype, sizeof(Packet), 0);
  send(socketClient, (char *)&msg_size, sizeof(int), 0);
  send(socketClient, msg_chat.c_str(), msg_size, 0);
  while (true) {
    getline(cin, msg_chat);
    if (msg_chat == "/exit") break;
    msg_size = msg_chat.size();
    Packet packettype = P_Message;
    send(socketClient, (char *)&packettype, sizeof(Packet), 0);
    send(socketClient, (char *)&msg_size, sizeof(int), 0);
    send(socketClient, msg_chat.c_str(), msg_size, 0);
  }
  packettype = P_ExitToMenu;
  send(socketClient, (char *)&packettype, sizeof(Packet), 0);
}

vector<vector<char> > get_board() {
  vector<vector<char> > ans(size_board, vector<char>(size_board));
  for (int i = 0; i < size_board; i++) {
    recv(socketClient, ans[i].data(), sizeof(char) * size_board, 0);
  }
  return ans;
}

bool ProcessPacket(Packet packettype) {
  switch (packettype) {
    case P_ExitGame: {
      int choose;
      vector<vector<char> > board = get_board();
      recv(socketClient, (char *)&choose, sizeof(int), 0);
      draw_game(board, choose);
      cout << "You will be disconnected from the game in 5 seconds.\n" << endl;
      sleep(5);
    } break;
    case P_PlayGame: {
      int check_progress;  // игрок ходит или нет
      vector<vector<char> > board = get_board();
      recv(socketClient, (char *)&check_progress, sizeof(int), 0);
      draw_game(board, check_progress);
    } break;
    case P_Message:  // получение от сервера ссобщений
    {
      int msg_size;
      recv(socketClient, (char *)&msg_size, sizeof(int), 0);
      char *msg = new char[msg_size + 1];
      msg[msg_size] = '\0';
      recv(socketClient, msg, msg_size, 0);
      cout << msg << endl;
      ;
      delete[] msg;
    } break;
    case P_Menu:  // выход в меню
    {
      draw_menu();
      int variant = get_variant(0, 2);
      switch (variant) {
        case 0: {
          draw_header();
          cout << "See you soon!" << endl;
          packettype = P_ExitProgramm;
          send(socketClient, (char *)&packettype, sizeof(Packet), 0);
          sleep(5);
          return false;
        } break;
        case 1: {
          Packet packettype = P_PlayGame;
          send(socketClient, (char *)&packettype, sizeof(Packet), 0);
        } break;
        case 2: {
          thread(chat_message).detach();
        } break;
      }
    } break;

    default: {
    } break;
  }
  return true;
}

// поток управления пакетов
void ClientHandler() {
  Packet packettype;
  while (true) {
    recv(socketClient, (char *)&packettype, sizeof(Packet), 0);
    if (!ProcessPacket(packettype)) {
      break;
    }
  }
  close(socketClient);
}

// client
int client(int argc, char *argv[]) {
  int PORT;
  string Ip = "127.0.0.1";
  string cpy_PORT = "8200";
  // проверка на правильный ввод командной строки
  if (argc != 3)
    return error(
        "Launch error\nPlease write in the format: ./client --connect \"IP "
        "Number:Port Number\"");
  else if (strcmp(argv[1], "--connect") != 0) {
    cout << "./server: unrecognized option \"" << argv[1] << "\"" << endl;
    return error("usage: ./client --connect");
  } else {
    string check = argv[2];
    int finded;
    if (static_cast<size_t>(finded = check.find(":")) == string::npos) {
      return error(
          "Invalid connect format, please usage format: \"127.0.0.1:PORT\"");
    } else {
      string cpy_Ip = check.substr(0, finded);
      if (Ip != cpy_Ip)
        return error("Sorry, only IP is available now: 127.0.0.1");
      else
        cpy_PORT = check.substr(finded + 1, check.size()).c_str();
    }

    if (to_string(atoi(cpy_PORT.c_str())) != cpy_PORT ||
        atoi(cpy_PORT.c_str()) < 1024 || atoi(cpy_PORT.c_str()) > 65535) {
      return error(
          "Invalid port format, please enter a port between 1024 and 65535");
    }
  }
  PORT = atoi(cpy_PORT.c_str());

  struct sockaddr_in caddr = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = inet_addr(Ip.c_str()),  // брать из адресной строки
      .sin_port = htons(PORT)  // брать из адресной строки
  };
  cout << "TicTacToe client version 1.0.0" << endl;
  cout << "Connecting server at " << argv[2] << "...." << endl;
  socketClient = socket(AF_INET, SOCK_STREAM, 0);
  if (connect(socketClient, (struct sockaddr *)&caddr, sizeof(caddr)) != 0) {
    string err =
        "Error: failed to connect to server " + static_cast<string>(argv[2]);
    return error(err.c_str());
  }
  cout << "\nWelcome to the TicTacToe server version 1.0.0!\n";
  sleep(2);

  thread ClinetThread(ClientHandler);
  ClinetThread.join();

  return 0;
}

int main(int argc, char *argv[]) { return client(argc, argv); }