######################################################server code
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <cctype>  // 공백 제거 및 대소문자 비교를 위한 헤더
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

constexpr int BUF_SIZE = 1024;
constexpr int PORT = 9090;
const std::string CSV_FILE = "workouts.csv";  // 기존 운동 목록 파일
const std::string PLAYLIST_FILE = "workout_playlist.csv";  // 나만의 운동 리스트 파일
const std::string DATE_LOG_FILE = "date_logs.csv";  // 운동 기록 파일

// 에러 처리 함수
void error_handling(const std::string& message) {
    std::cerr << message << std::endl;
    WSACleanup();
    exit(1);
}

// 문자열에서 앞뒤 공백을 제거하는 함수
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    size_t last = str.find_last_not_of(" \t\n\r");
    if (first == std::string::npos || last == std::string::npos) {
        return "";
    }
    return str.substr(first, (last - first + 1));
}

// 운동을 workout_playlist.csv에 추가하는 함수
void add_to_playlist(const std::string& exercise) {
    std::ofstream file(PLAYLIST_FILE, std::ios::app);
    if (file.is_open()) {
        file << exercise << "\n";  // 운동 이름만 저장
        file.close();
        std::cout << exercise << " 운동이 나만의 운동 리스트에 추가되었습니다.\n";
    }
    else {
        std::cerr << "운동 리스트 파일을 열 수 없습니다." << std::endl;
    }
}

// 운동 데이터를 로드하는 함수
void load_workout_data(std::unordered_map<std::string, std::vector<std::string>>& part_to_exercises,
    std::unordered_map<std::string, std::string>& exercise_to_description) {
    std::ifstream file(CSV_FILE);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "CSV 파일을 열 수 없습니다." << std::endl;
        exit(1);
    }

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string 부위, 운동, 설명;

        std::getline(ss, 부위, ',');
        std::getline(ss, 운동, ',');
        std::getline(ss, 설명, ',');

        part_to_exercises[부위].push_back(trim(운동));  // 운동 이름에 앞뒤 공백 제거
        exercise_to_description[trim(운동)] = trim(설명);  // 설명에도 공백 제거
    }
    file.close();
}

// 운동 목록에서 운동을 추가하는 명령어를 처리하는 함수
void handle_add_to_playlist(const std::string& command, std::unordered_map<std::string, std::vector<std::string>>& part_to_exercises, std::string& response) {
    // 명령어에서 운동 이름을 추출
    size_t pos = command.find(" ");
    if (pos == std::string::npos) {
        response = "운동 추가 명령 형식이 잘못되었습니다. 형식: 운동 추가 운동이름\n";
        return;
    }

    std::string exercise = (command.substr(pos + 1));  // 공백 제거
    response = exercise;
    // 운동이 기존 운동 목록에 있는지 확인
    bool found = false;
    for (const auto& part : part_to_exercises) {
        // 운동 이름을 정확히 비교 (공백 및 대소문자 구분 없이)
        for (const auto& existing_exercise : part.second) {
            if (trim(existing_exercise) == exercise) {  // 비교 전에 공백 제거
                found = true;
                break;
            }
        }
        if (found) break;
    }

    if (found) {
        add_to_playlist(exercise);
        response = exercise + " 운동이 나만의 운동 리스트에 추가되었습니다.\n";
    }
    else {
        response = exercise + " 운동은 운동 목록에 없습니다.\n";
    }
}

// 특정 날짜의 운동 기록을 삭제하는 함수
void delete_workout_date(const std::string& date, std::string& response) {
    std::ifstream date_file(DATE_LOG_FILE);
    if (!date_file.is_open()) {
        response = "운동 일자 기록 파일을 찾을 수 없습니다.\n";
        return;
    }

    std::vector<std::pair<std::string, std::string>> logs;
    std::string line;

    // 기록 로드
    bool record_found = false;
    while (std::getline(date_file, line)) {
        std::stringstream ss(line);
        std::string log_date, workout;
        std::getline(ss, log_date, ',');
        std::getline(ss, workout);

        // 삭제 대상 날짜가 아니라면 유지
        if (trim(log_date) != trim(date)) {
            logs.emplace_back(trim(log_date), trim(workout));
        }
        else {
            record_found = true;
        }
    }
    date_file.close();

    // 삭제 대상 기록이 없으면 메시지 반환
    if (!record_found) {
        response = date + " 기록을 찾을 수 없습니다.\n";
        return;
    }

    // 기록 업데이트
    std::ofstream out_file(DATE_LOG_FILE);
    for (const auto& log : logs) {
        out_file << log.first << "," << log.second << "\n";
    }
    out_file.close();

    response = date + " 기록이 삭제되었습니다.\n";
}

// 운동 리스트 보기
std::string view_playlist() {
    std::ifstream file(PLAYLIST_FILE);
    std::string line, response;

    if (file.is_open()) {
        response = "나만의 운동 리스트:\n";
        while (std::getline(file, line)) {
            response += line + "\n";
        }
        file.close();
    }
    else {
        response = "운동 리스트 파일을 열 수 없습니다.\n";
    }

    return response;
}

// 운동 기록을 보여주는 함수
std::string view_workout_dates() {
    std::ifstream date_file(DATE_LOG_FILE);
    std::string line, response;

    if (date_file.is_open()) {
        std::vector<std::pair<std::string, std::string>> logs;
        while (std::getline(date_file, line)) {
            std::stringstream ss(line);
            std::string date, workout;
            std::getline(ss, date, ',');
            std::getline(ss, workout);
            logs.emplace_back(date, workout);
        }
        date_file.close();

        // 날짜 오름차순으로 정렬
        std::sort(logs.begin(), logs.end());

        // 양식에 맞춰 출력 문자열 구성
        response = "운동 기록:\n";
        for (const auto& log : logs) {
            response += log.first + " - " + log.second + "\n";
        }
    }
    else {
        response = "운동 일자 기록 파일을 찾을 수 없습니다.\n";
    }

    return response;
}

void add_workout_record(const std::string& date, const std::string& workout) {
    std::ofstream date_file(DATE_LOG_FILE, std::ios::app);
    if (date_file.is_open()) {
        date_file << date << "," << workout << "\n";
        date_file.close();
        std::cout << date << " 날짜의 " << workout << " 운동 기록이 추가되었습니다.\n";
    }
    else {
        std::cerr << "운동 기록 파일을 열 수 없습니다.\n";
    }
}

// 운동 삭제 함수
void delete_from_playlist(const std::string& exercise) {
    std::ifstream file(PLAYLIST_FILE);
    std::vector<std::string> exercises;
    std::string line;

    bool found = false;
    if (file.is_open()) {
        // 파일을 읽고 삭제하려는 운동을 제외한 모든 항목을 벡터에 저장
        while (std::getline(file, line)) {
            if (trim(line) != trim(exercise)) {  // 공백 제거하여 정확히 비교
                exercises.push_back(line);
            }
            else {
                found = true;
            }
        }
        file.close();
    }

    // 삭제할 운동이 존재할 경우 파일에 다시 저장
    if (found) {
        std::ofstream out_file(PLAYLIST_FILE);
        for (const auto& ex : exercises) {
            out_file << ex << "\n";
        }
        std::cout << exercise << " 운동이 나만의 운동 리스트에서 삭제되었습니다.\n";
    }
    else {
        std::cout << "삭제하려는 운동을 찾을 수 없습니다.\n";
    }
}

// 운동 리스트 초기화 함수
void clear_playlist() {
    std::ofstream file(PLAYLIST_FILE, std::ios::trunc); // 내용만 지우고 파일은 유지
    if (file.is_open()) {
        file.close();
        std::cout << "나만의 운동 리스트가 초기화되었습니다.\n";
    }
    else {
        std::cerr << "운동 리스트 파일을 초기화할 수 없습니다.\n";
    }
}

// 날짜 기록 초기화 함수
void clear_date_logs() {
    std::ofstream file(DATE_LOG_FILE, std::ios::trunc); // 내용만 지우고 파일은 유지
    if (file.is_open()) {
        file.close();
        std::cout << "운동 기록이 초기화되었습니다.\n";
    }
    else {
        std::cerr << "운동 기록 파일을 초기화할 수 없습니다.\n";
    }
}


// 도움말 함수
std::string help() {
    return "사용 가능한 명령어:\n"
        "목록 - 운동 부위 목록을 출력합니다.\n"
        "운동이름 예)푸시 업을 입력하면 푸시 업에 대한 설명을 출력합니다.\n"
        "운동추가 운동이름 - 나만의 운동 리스트에 운동을 추가합니다.\n"
        "운동삭제 운동이름 - 나만의 운동 리스트에서 운동을 삭제합니다.\n"
        "리스트보기 - 나만의 운동 리스트를 확인합니다.\n"
        "기록보기 - 나의 운동 기록을 확인합니다.\n"
        "리스트초기화 - 나만의 운동 리스트를 초기화합니다.\n";
        "기록초기화 - 나의 운동 기록을 초기화합니다.\n";

}

// 클라이언트 처리 함수
void handle_client(SOCKET clnt_sock,
    std::unordered_map<std::string, std::vector<std::string>>& part_to_exercises,
    std::unordered_map<std::string, std::string>& exercise_to_description) {
    char message[BUF_SIZE];

    while (true) {
        int str_len = recv(clnt_sock, message, BUF_SIZE - 1, 0);
        if (str_len <= 0) break;

        message[str_len] = '\0';
        std::string command(message);
        command.erase(command.find_last_not_of(" \n\r\t") + 1);
        std::string response;

        if (command == "목록") {
            std::unordered_set<std::string> unique_parts;
            for (const auto& entry : part_to_exercises) {
                unique_parts.insert(entry.first);
            }
            for (const auto& part : unique_parts) {
                response += part + "\n";
            }
        }
        else if (exercise_to_description.find(command) != exercise_to_description.end()) {
            response = command + " 설명: " + exercise_to_description[command];
        }
        else if (part_to_exercises.find(command) != part_to_exercises.end()) {
            for (const auto& exercise : part_to_exercises[command]) {
                response += exercise + "\n";
            }
        }
        else if (command == "리스트보기") {
            response = view_playlist();
        }
        else if (command == "기록보기") {
            response = view_workout_dates();
        }
        else if (command.find("운동추가") == 0) {
            handle_add_to_playlist(command, part_to_exercises, response);
        }
        else if (command.find("운동삭제") == 0) {  // 정확히 운동 삭제 명령인지 검사
            size_t pos = command.find(" ");
            if (pos != std::string::npos) {
                delete_from_playlist(command.substr(pos + 1));  // 명령어와 운동 이름 분리
                response = "운동이 삭제되었습니다.\n";
            }
            else {
                response = "운동 삭제 명령 형식이 잘못되었습니다.\n";
            }
        }

        else if (command == "리스트초기화") {
            clear_playlist();
            response = "운동 리스트가 초기화되었습니다.\n";
        }
        else if (command == "기록초기화") {
            clear_date_logs();
            response = "날짜 기록이 초기화되었습니다.\n";
        }
        else if (command.find("기록삭제") == 0) {
            size_t pos = command.find(" ");
            if (pos != std::string::npos) {
                std::string date_to_delete = command.substr(pos + 1);  // 날짜 추출
                delete_workout_date(trim(date_to_delete), response);  // 삭제 함수 호출
            }
            else {
                response = "기록삭제 명령 형식이 잘못되었습니다. 형식: 기록삭제 YYYY-MM-DD\n";
            }
        }
        else if (command.find("기록추가") == 0) {
            size_t pos = command.find(" ");
            if (pos != std::string::npos) {
                std::string details = command.substr(pos + 1);
                size_t comma_pos = details.find(" ");
                if (comma_pos != std::string::npos) {
                    std::string date = details.substr(0, comma_pos);
                    std::string workout = details.substr(comma_pos + 1);
                    add_workout_record(date, workout);
                    response = date + " 날짜의 " + workout + " 운동 기록이 추가되었습니다.\n";
                }
                else {
                    response = "기록 추가 명령 형식이 잘못되었습니다. 형식: 기록추가 YYYY-MM-DD 운동이름\n";
                }
            }
        }
        else {
            response = help();
        }

        send(clnt_sock, response.c_str(), response.size(), 0);
    }

    closesocket(clnt_sock);
}

int main() {
    WSADATA wsaData;
    SOCKET serv_sock, clnt_sock;
    sockaddr_in serv_addr, clnt_addr;
    int clnt_addr_size;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        error_handling("WSAStartup() 오류");
    }

    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == INVALID_SOCKET) {
        error_handling("소켓 생성 오류");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (bind(serv_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        error_handling("bind() 오류");
    }

    if (listen(serv_sock, 5) == SOCKET_ERROR) {
        error_handling("listen() 오류");
    }

    std::unordered_map<std::string, std::vector<std::string>> part_to_exercises;
    std::unordered_map<std::string, std::string> exercise_to_description;

    load_workout_data(part_to_exercises, exercise_to_description);

    while (true) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == INVALID_SOCKET) {
            error_handling("accept() 오류");
        }

        handle_client(clnt_sock, part_to_exercises, exercise_to_description);
    }

    closesocket(serv_sock);
    WSACleanup();
    return 0;
}

######################################################client code
#include <iostream>
#include <string>
#include <winsock2.h>   
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")  // Winsock 라이브러리 연결

constexpr int BUF_SIZE = 1024;

void error_handling(const std::string& message) {
    std::cerr << message << std::endl;
    WSACleanup();
    exit(1);
}

int main() {
    WSADATA wsaData;
    SOCKET sock;
    sockaddr_in server_addr;
    char message[BUF_SIZE];
    int str_len;

    // Winsock 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        error_handling("WSAStartup() 실패");
    }

    // 소켓 생성
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        error_handling("socket() 실패");
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    // 로컬 IP 주소로 설정 (127.0.0.1)
    if (InetPton(AF_INET, L"127.0.0.1", &server_addr.sin_addr) != 1) {
        error_handling("InetPton() 실패");
    }
    server_addr.sin_port = htons(9090);  // 서버 포트 설정

    // 서버에 연결
    if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
        error_handling("connect() 실패");
    }
    std::cout << "서버에 연결되었습니다. (도움말을 입력하여 서버 명령어를 확인할 수 있습니다.) \n";
    // 서버와의 통신을 지속적으로 수행할 루프
    while (true) {
        std::cout << "\n$";
        std::string user_input;
        std::getline(std::cin, user_input);

        if (user_input == "종료") {
            send(sock, user_input.c_str(), user_input.size(), 0);
            std::cout << "연결을 종료합니다.\n";
            break;
        }

        // 명령어 전송
        user_input += "\n";  // 명령어 뒤에 엔터 키 추가
        send(sock, user_input.c_str(), user_input.size(), 0);

        // 서버로부터 응답 수신
        std::cout << "\n서버 응답:\n";
        while ((str_len = recv(sock, message, BUF_SIZE - 1, 0)) > 0) {
            message[str_len] = '\0';
            std::cout << message;

            // 현재 응답이 완전히 수신되었음을 표시하고 다음 명령을 받기 위해 루프 탈출
            if (str_len < BUF_SIZE - 1) {

                break;
            }
        }
    }

    // 소켓 종료 및 Winsock 정리
    closesocket(sock);
    WSACleanup();
    return 0;
}
