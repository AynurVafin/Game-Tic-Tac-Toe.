#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

using namespace std;

int Connections[100];  // все соединения
const int all_boards = 100;
int CounterClient = 0;
int prost_ind = 0;
vector<vector<int> > sessions(all_boards);
int sess_now = 0;
const int size_board = 3;
int win_combin[3];
int who_chat[100] = {false};  // кто сидит в чате
queue<int> ready_game;        // кто готов играть
int user_sess[100] = {0};

typedef vector<vector<vector<char> > > v3;
v3 board(all_boards,
         vector<vector<char> >(size_board, vector<char>(size_board)));
int who_go[all_boards];  // кто ходит

int error(const char* msg) {
  perror(msg);
  return 1;
}

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

// заполняем игровые доски нач. знач.
void fill_init_boards(int i) {
  for (int j = 0; j < size_board; j++) {
    for (int k = 0; k < size_board; k++) {
      board[i][j][k] = j * 3 + k + 1 + '0';
    }
  }
}

// проверка на ничью
bool check_draw(int session) {
  for (int i = 0; i < size_board; i++) {
    for (int j = 0; j < size_board; j++) {
      if (board[session][i][j] != 'X' && board[session][i][j] != 'O')
        return false;
    }
  }
  return true;
}

bool check_win(int session) {
  if (board[session][0][0] == board[session][1][1] &&
      board[session][0][0] == board[session][2][2]) {
    win_combin[0] = 1;
    win_combin[1] = 5;
    win_combin[2] = 9;
    return true;
  }
  if (board[session][0][2] == board[session][1][1] &&
      board[session][0][2] == board[session][2][0]) {
    win_combin[0] = 3;
    win_combin[1] = 5;
    win_combin[2] = 7;
    return true;
  }
  if (board[session][0][0] == board[session][0][1] &&
      board[session][0][0] == board[session][0][2]) {
    win_combin[0] = 1;
    win_combin[1] = 2;
    win_combin[2] = 3;
    return true;
  }
  if (board[session][1][0] == board[session][1][1] &&
      board[session][1][0] == board[session][1][2]) {
    win_combin[0] = 4;
    win_combin[1] = 5;
    win_combin[2] = 6;
    return true;
  }
  if (board[session][2][0] == board[session][2][1] &&
      board[session][2][0] == board[session][2][2]) {
    win_combin[0] = 7;
    win_combin[1] = 8;
    win_combin[2] = 9;
    return true;
  }
  if (board[session][0][0] == board[session][1][0] &&
      board[session][0][0] == board[session][2][0]) {
    win_combin[0] = 1;
    win_combin[1] = 4;
    win_combin[2] = 7;
    return true;
  }
  if (board[session][0][1] == board[session][1][1] &&
      board[session][0][1] == board[session][2][1]) {
    win_combin[0] = 2;
    win_combin[1] = 5;
    win_combin[2] = 8;
    return true;
  }
  if (board[session][0][2] == board[session][1][2] &&
      board[session][0][2] == board[session][2][2]) {
    win_combin[0] = 3;
    win_combin[1] = 6;
    win_combin[2] = 9;
    return true;
  }
  return false;
}

void push_board(int index, int session) {
  for (int i = 0; i < size_board; i++) {
    send(Connections[index], board[session][i].data(),
         sizeof(char) * size_board, 0);
  }
}

void push_msg(string msg_chat, int session, int i) {
  int msg_size = msg_chat.size();
  Packet msgtype = P_Message;
  send(Connections[sessions[session][i]], (char*)&msgtype, sizeof(Packet), 0);
  send(Connections[sessions[session][i]], (char*)&msg_size, sizeof(int), 0);
  send(Connections[sessions[session][i]], msg_chat.c_str(), msg_size, 0);
}

void push_play_game(int session) {
  Packet msgtype = P_PlayGame;
  send(Connections[sessions[session][0]], (char*)&msgtype, sizeof(Packet), 0);
  send(Connections[sessions[session][1]], (char*)&msgtype, sizeof(Packet), 0);
  push_board(sessions[session][0], session);
  push_board(sessions[session][1], session);
}

void push_status_exit_game(int i, int session, int msg) {
  Packet msgtype = P_ExitGame;
  send(Connections[sessions[session][i]], (char*)&msgtype, sizeof(Packet), 0);
  push_board(sessions[session][i], session);
  send(Connections[sessions[session][i]], (char*)&msg, sizeof(int), 0);
  msgtype = P_Menu;
  send(Connections[sessions[session][i]], (char*)&msgtype, sizeof(Packet), 0);
}

bool ProcessPacket(int index, Packet packettype) {
  int session = user_sess[index];
  switch (packettype) {
    case P_Check: {
      int variant;
      recv(Connections[index], (char*)&variant, sizeof(int), 0);
      variant--;
      char check = board[session][variant / size_board][variant % size_board];
      int checker = (check != 'X' && check != 'O');
      send(Connections[index], (char*)&checker, sizeof(int), 0);

      if (!checker) break;
      cout << "S" << session + 1 << "P" << index + 1 << "-" << variant + 1
           << endl;
      if (index == sessions[session][0])
        board[session][variant / size_board][variant % size_board] = 'X';
      else
        board[session][variant / size_board][variant % size_board] = 'O';

      if (check_win(session)) {
        cout << "S" << session + 1 << "P" << index + 1 << " wins with ";
        for (int i = 0; i < 3; i++) {
          cout << win_combin[i];
          if (i != 2)
            cout << "-";
          else
            cout << "!" << endl;
        }

        if (index == sessions[session][0]) {
          push_status_exit_game(0, session, -1);
          push_status_exit_game(1, session, -2);
        } else {
          push_status_exit_game(1, session, -1);
          push_status_exit_game(0, session, -2);
        }
        break;
      }
      if (check_draw(session)) {
        cout << "S" << session + 1 << "P" << index + 1 << "-draw" << endl;
        push_status_exit_game(0, session, -3);
        push_status_exit_game(1, session, -3);
        break;
      }
      push_play_game(session);
      send(Connections[sessions[session][0]], (char*)&(who_go[session]),
           sizeof(int), 0);
      who_go[session] = (who_go[session] + 1) % 2;
      send(Connections[sessions[session][1]], (char*)&(who_go[session]),
           sizeof(int), 0);
    } break;
    case P_PlayGame: {
      if (ready_game.size() == 1) {
        user_sess[index] = sess_now;
        user_sess[ready_game.front()] = sess_now;
        ready_game.pop();
        session = sess_now;
        fill_init_boards(session);
        sessions[session].push_back(index);
        cout << "S" << session + 1 << "P" << index + 1 << "-connection game"
             << endl;
        push_msg(
            "The second player connected!\nStarting the game...\nYou are "
            "playing crosses (X). Good luck!",
            session, 0);
        push_msg(
            "You are the second player!\nStarting the game...\nYou are playing "
            "noughts (O). Good luck!",
            session, 1);
        cout << "Starting game session " << session + 1 << "..." << endl;
        sess_now++;
        push_play_game(session);
        srand(time(nullptr));
        who_go[session] = rand() % 2;
        send(Connections[sessions[session][0]], (char*)&(who_go[session]),
             sizeof(int), 0);
        who_go[session] = (who_go[session] + 1) % 2;
        send(Connections[sessions[session][1]], (char*)&(who_go[session]),
             sizeof(int), 0);
      } else {
        ready_game.push(index);
        cout << "S" << sess_now + 1 << "P" << index + 1 << "-connection game"
             << endl;
        sessions[sess_now].push_back(index);
        push_msg(
            "You are the first player!\nPlease wait for the other player to "
            "connect...",
            sess_now, 0);
      }
    } break;
    case P_Message: {
      int msg_size;
      recv(Connections[index], (char*)&msg_size, sizeof(int), 0);
      char* msg = new char[msg_size + 1];
      msg[msg_size] = '\0';
      recv(Connections[index], msg, msg_size, 0);
      who_chat[index] = true;
      string msg_chat = "Player " + to_string(index + 1) + ": " + msg;
      msg_size = msg_chat.size();
      for (int i = 0; i < CounterClient; i++) {
        if (i == index || !who_chat[i]) {
          continue;
        }
        Packet msgtype = P_Message;
        send(Connections[i], (char*)&msgtype, sizeof(Packet), 0);
        send(Connections[i], (char*)&msg_size, sizeof(int), 0);
        send(Connections[i], msg_chat.c_str(), msg_size, 0);
      }
      delete[] msg;
    } break;
    case P_ExitProgramm: {
      cout << "Player " << to_string(index + 1) << " disconnected..." << endl;
      return false;
    } break;

    case P_ExitToMenu: {
      who_chat[index] = false;
      Packet packet = P_Menu;
      send(Connections[index], (char*)&packet, sizeof(Packet), 0);
    } break;
    default:
      cout << "Unrecognized packet: " << packettype << endl;
      break;
  }
  return true;
}

void ServerHandler() {
  int index = prost_ind;

  Packet packettype;
  while (true) {
    recv(Connections[index], (char*)&packettype, sizeof(Packet), 0);

    if (!ProcessPacket(index, packettype)) {
      break;
    }
  }
  close(Connections[index]);
}

// server
int server(int argc, char* argv[]) {
  int PORT = 8200;
  // проверка на правильный ввод командной строки
  if (argc != 3)
    return error(
        "Launch error, please write in the format: ./server --port \"Port "
        "Number\"");
  else if (strcmp(argv[1], "--port") != 0) {
    cout << "./server: unrecognized option \"" << argv[1] << "\"" << endl;
    return error("usage: ./server --port");
  } else if (to_string(atoi(argv[2])) != argv[2] || atoi(argv[2]) < 1024 ||
             atoi(argv[2]) > 65535) {
    return error(
        "Invalid port format, please enter a port between 1024 and 65535");
  }
  PORT = atoi(argv[2]);
  struct sockaddr_in saddr = {.sin_family = AF_INET,
                              .sin_addr.s_addr = inet_addr("127.0.0.1"),
                              .sin_port = htons(PORT)};
  cout << "TicTacToe server version 1.0.0" << endl;
  int sizeof_saddr = sizeof(saddr);
  int socketServer = socket(AF_INET, SOCK_STREAM, 0);
  int optval = 1;
  setsockopt(socketServer, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &optval,
             sizeof(optval));
  if (socketServer == -1) {
    cout << "Not able to create socket" << endl;
    return -1;
  }
  bind(socketServer, (struct sockaddr*)&saddr, sizeof(saddr));
  listen(socketServer, SOMAXCONN);
  cout << "Listening for incoming connections on port " << PORT << "..."
       << endl;  // ljqwdwql

  int newConnection;
  for (int i = 0; i < 100; i++) {
    newConnection = accept(socketServer, (struct sockaddr*)&saddr,
                           (socklen_t*)&sizeof_saddr);
    if (newConnection < 0) {
      string err =
          "Error connection player/fan number " + to_string(CounterClient + 1);
      return error(err.c_str());
    } else {
      cout << "Player " << to_string(CounterClient + 1) << " connected..."
           << endl;
      Packet packet = P_Menu;
      send(newConnection, (char*)&packet, sizeof(Packet), 0);

      Connections[i] = newConnection;
      CounterClient++;

      prost_ind = i;
      thread ServerThread(ServerHandler);
      ServerThread.detach();
    }
  }
  return 0;
}

int main(int argc, char* argv[]) { return server(argc, argv); }