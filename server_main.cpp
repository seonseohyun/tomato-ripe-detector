#include <iostream>      // 표준 입출력 스트림 사용 (cout, cerr 등)
#include <fstream>       // 파일 입출력을 위한 ifstream, ofstream 클래스
#include <cstring>       // 문자열 처리 함수들 (memset, strcmp 등)
#include <unistd.h>      // POSIX 함수들 (read, write, close 등)
#include <arpa/inet.h>   // IP 주소 변환 함수 (inet_pton 등)
#include <sys/socket.h>  // 소켓 API 정의
#include <chrono>        // 시간 기반 파일명 생성을 위한 헤더
#include <ctime>         // 시간 변환을 위한 헤더
#include <sstream>       // 문자열 스트림 처리
#include <sys/stat.h>    // mkdir 사용을 위한 헤더
#include <sys/types.h>   // mode_t

// --- 상수 정의 ---
#define CLIENT_PORT 12345                 // 클라이언트가 연결해올 포트 번호
#define PYTHON_SERVER_IP "0.0.0.0"        // Python 서버의 IP 주소 (로컬 테스트 용)
#define BUFFER_SIZE 4096                  // 데이터 전송 시 사용할 버퍼 크기 (4KB)

#include <thread>

#include <mysql/mysql.h>
#include <nlohmann/json.hpp>

using ordered_json = nlohmann::ordered_json;
using json = nlohmann::json;
using namespace std;

// Base64 디코딩 함수 정의 // Base64로 인코딩된 문자열을 다시 원래의 이진 데이터(예: 이미지, 파일, 바이너리 등)**로 되돌리는 과정
static const string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

const char* host = "10.10.20.123";
const char* user = "pipipi";
const char* password = "1234";
const char* dbname = "TOMATO";
unsigned int port = 3306;
int python_fd = 0;


class handle_class{
    
    private:

    public:
    handle_class(){

    };
    
// 서버가 클라이언트에게 응답 JSON 전송 + 디버깅
void send_json_response(int client_fd, const string& res_str){

        int len = htonl(res_str.size());
        cout << "[DEBUG] 서버->클라이언트 JSON 길이: " << res_str.size() << endl;
        cout << "[DEBUG] 서버->클라이언트 JSON 내용: " << res_str << endl;
        send(client_fd, &len, sizeof(len), 0);
        send(client_fd, res_str.c_str(), res_str.size(), 0);
        cout << "전송완료!: " << endl;
}
// 로그인 메서드 함수~
string handling_login(const string& json_data) {
    auto received = nlohmann::json::parse(json_data);
    string id = received["id"];
    string pw = received["password"];

    MYSQL* conn = mysql_init(nullptr);
    if (!mysql_real_connect(conn, host, user, password, dbname, port, nullptr, 0)) {
        cerr << "DB 연결 실패: " << mysql_error(conn) << endl;
        ordered_json fail = {
            {"protocol", "login_1"},
            {"message", "DB 연결 실패"}
        };
        return fail.dump();
    }

    string query = "SELECT * FROM OWNER WHERE USERNAME='" + id + "' AND PASSWORD='" + pw + "'";
    cout << "로그인 쿼리문 : " << query << endl;

    if (mysql_query(conn, query.c_str()) != 0) {
        cerr << "쿼리 실패: " << mysql_error(conn) << endl;
        mysql_close(conn);
        ordered_json fail = {
            {"protocol", "login_1"},
            {"message", "쿼리 실패"}
        };
        return fail.dump();
    }

    MYSQL_RES* login_result = mysql_store_result(conn);
    if (login_result == nullptr || mysql_num_rows(login_result) == 0) {
        mysql_free_result(login_result);
        mysql_close(conn);
        ordered_json fail = {
            {"protocol", "login_1"},
            {"message", "아이디 또는 비밀번호가 올바르지 않습니다"}
        };
        return fail.dump();
    }
    mysql_free_result(login_result);

    // 농장 정보 조회
    query = "SELECT * FROM FARM_INFO WHERE OWNER='" + id + "'";
    cout << "농장조회 쿼리문 : " << query << endl;

    if (mysql_query(conn, query.c_str()) != 0) {
        cerr << "쿼리 실패: " << mysql_error(conn) << endl;
        mysql_close(conn);
        ordered_json fail = {
            {"protocol", "login_1"},
            {"message", "쿼리 실패"}
        };
        return fail.dump();
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    
    // 보내줄 문자열
    json send;

    send["protocol"] = "login_0";
    send["farms"] = json::array();

    // DB에서 쿼리문써서 받은 값을 한줄씩 읽기!
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        json tmp;
        // 순서대로 서현이랑 맞춘 KEY값에 저장
        tmp["farm_num"] = row[0];     // FARM_NUM
        tmp["farm_name"] = row[1];    // FARM_NAME
        tmp["farm_address"] = row[2]; // FARM_ADDRESS
        tmp["farm_area"] = row[4];    // FARM_AREA

        // farms 배열에 푸쉬백. append랑 똑같아요
        send["farms"].push_back(tmp);
    }

    mysql_free_result(result);
    mysql_close(conn);

    return send.dump();
}
// 클라이언트 이미지 수신 및 DB 저장 처리
void receiving_image(int client_fd, json receive) {
    uint64_t fileSize = receive["file_size"];
    int farm_num = receive["farm_num"];
    std::cout << "리시빙 이미지에 들어왔어요" << std::endl;

    // 현재 시간 → 문자열 변환
    auto now = std::chrono::system_clock::now();
    time_t now_c = std::chrono::system_clock::to_time_t(now);
    tm* now_tm = localtime(&now_c);

    char time_buf[10], date_buf[11];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", now_tm);
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", now_tm);
    std::string date_str = date_buf;

    // 파일명 생성
    std::ostringstream filename_stream;
    filename_stream << "img_" << (now_tm->tm_year + 1900)
                    << (now_tm->tm_mon + 1) << now_tm->tm_mday << "_"
                    << now_tm->tm_hour << now_tm->tm_min << now_tm->tm_sec << ".jpg";
    std::string filename_name = filename_stream.str();

    // 경로 생성
    std::string folder, full_path, db_img_path;
    get_image_save_paths("images", to_string(farm_num), date_str, "before", filename_name, folder, full_path, db_img_path);

    // 이미지 저장
    if (!save_binary_file(client_fd, full_path, fileSize)) {
        send_json_response(client_fd, R"({"protocol":"image_1","message":"파일 저장 실패"})");
        return;
    }

    std::cout << "[INFO] 이미지 저장 완료: " << full_path << "\n" << std::endl;

    // DB 저장
    if (!insert_to_db(db_img_path, farm_num)) {
        return;
    }
    int img_num = get_image_num(db_img_path);
    // 파이썬 서버 전송
    send_image_to_python(python_fd, full_path, filename_name, fileSize, farm_num, img_num);

    // 응답 전송
    send_json_response(client_fd, R"({"protocol":"image_0","message":"이미지 저장 성공"})");
}
// 디렉토리 생성 
void create_directories(const string& path) {
    size_t pos = 0;
    do {
        pos = path.find('/', pos + 1);
        mkdir(path.substr(0, pos).c_str(), 0755);
    } while (pos != string::npos);
}
// 이진 파일 저장 
bool save_binary_file(int fd, const string& full_path, uint64_t fileSize) {
    ofstream output(full_path, ios::binary);
    if (!output) {
        cerr << "[ERROR] 파일 저장 실패: " << full_path << "\n";
        return false;
    }

    char buffer[4096];
    uint64_t total = 0;
    while (total < fileSize) {
        int toRead = min((uint64_t)4096, fileSize - total);
        int bytes = read(fd, buffer, toRead);
        if (bytes <= 0) break;
        output.write(buffer, bytes);
        total += bytes;
    }
    output.close();
    return true;
}
// DB 삽입 
bool insert_to_db(const string& db_img_path, int farm_num) {
    MYSQL* conn = mysql_init(nullptr);
    if (!mysql_real_connect(conn, host, user, password, dbname, port, nullptr, 0)) {
        cerr << "[ERROR] DB 연결 실패: " << mysql_error(conn) << "\n";
        return false;
    }

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    string insert_query = "INSERT INTO FARM_IMG (IMG_PATH, FARM_NUM) VALUES (?, ?)";

    if (mysql_stmt_prepare(stmt, insert_query.c_str(), insert_query.length()) != 0) {
        cerr << "[ERROR] Prepare 실패: " << mysql_error(conn) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return false;
    }

    MYSQL_BIND bind[2] = {};
    
    // IMG_PATH
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)db_img_path.c_str();
    bind[0].buffer_length = db_img_path.length();

    // FARM_NUM
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = (void*)&farm_num;
    bind[1].is_unsigned = 0;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        cerr << "[ERROR] 파라미터 바인딩 실패: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return false;
    }

    bool result = true;
    if (mysql_stmt_execute(stmt) != 0) {
        cerr << "[ERROR] 쿼리 실행 실패: " << mysql_stmt_error(stmt) << "\n";
        result = false;
    }

    mysql_stmt_close(stmt);
    mysql_close(conn);
    return result;
}


bool update_result_by_img_num(int img_num,
                              const string& db_img_path,
                              int total,
                              int red,
                              int yellow,
                              int green) {
    MYSQL* conn = mysql_init(nullptr);
    if (!mysql_real_connect(conn, host, user, password, dbname, port, nullptr, 0)) {
        cerr << "[ERROR] DB 연결 실패: " << mysql_error(conn) << "\n";
        return false;
    }

    string update_query = R"(
        UPDATE FARM_IMG
        SET IMG_PATH_ASK = ?, TOTAL_COUNT = ?,
            RED_TOMATO_COUNT = ?, YELLOW_TOMATO_COUNT = ?, GREEN_TOMATO_COUNT = ?
        WHERE IMG_NUM = ?
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, update_query.c_str(), update_query.length()) != 0) {
        cerr << "[ERROR] 쿼리 준비 실패: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return false;
    }

    MYSQL_BIND bind[6] = {};

    // IMG_PATH_ASK
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)db_img_path.c_str();
    bind[0].buffer_length = db_img_path.length();

    // TOTAL_COUNT
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = (void*)&total;

    // RED
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = (void*)&red;

    // YELLOW
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = (void*)&yellow;

    // GREEN
    bind[4].buffer_type = MYSQL_TYPE_LONG;
    bind[4].buffer = (void*)&green;

    // IMG_NUM (WHERE 조건)
    bind[5].buffer_type = MYSQL_TYPE_LONG;
    bind[5].buffer = (void*)&img_num;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        cerr << "[ERROR] 바인딩 실패: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return false;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        cerr << "[ERROR] 쿼리 실행 실패: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return false;
    }

    mysql_stmt_close(stmt);
    mysql_close(conn);
    return true;
}

int get_image_num(const string& db_img_path) {
    MYSQL* conn = mysql_init(nullptr);
    if (!mysql_real_connect(conn, host, user, password, dbname, port, nullptr, 0)) {
        cerr << "[ERROR] DB 연결 실패: " << mysql_error(conn) << "\n";
        return -1;
    }

    string find_query = "SELECT IMG_NUM FROM FARM_IMG WHERE IMG_PATH = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, find_query.c_str(), find_query.length()) != 0) {
        cerr << "[ERROR] 쿼리 준비 실패: " << mysql_error(conn) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return -1;
    }

    MYSQL_BIND bind_param = {};
    bind_param.buffer_type = MYSQL_TYPE_STRING;
    bind_param.buffer = (void*)db_img_path.c_str();
    bind_param.buffer_length = db_img_path.length();

    if (mysql_stmt_bind_param(stmt, &bind_param) != 0) {
        cerr << "[ERROR] 파라미터 바인딩 실패: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return -1;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        cerr << "[ERROR] 쿼리 실행 실패: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return -1;
    }

    int img_num = -1;
    MYSQL_BIND result_bind = {};
    result_bind.buffer_type = MYSQL_TYPE_LONG;
    result_bind.buffer = &img_num;
    result_bind.is_null = 0;
    result_bind.length = 0;

    if (mysql_stmt_bind_result(stmt, &result_bind) != 0) {
        cerr << "[ERROR] 결과 바인딩 실패: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return -1;
    }

    if (mysql_stmt_fetch(stmt) != 0) {
        cerr << "[ERROR] 결과 없음 또는 fetch 실패\n";
        img_num = -1;
    }

    mysql_stmt_close(stmt);
    mysql_close(conn);

    return img_num;
}

// 이미지 저장 경로 및 파일명 생성 유틸
void get_image_save_paths(const string& base_folder,const string& farm_num,const string& date_str,const string& subfolder, const string& base_filename, string& folder_out, string& full_path_out, string& db_img_path_out) {
    folder_out = base_folder + "/" + farm_num + "/" + date_str + "/" + subfolder;
    create_directories(folder_out);

    full_path_out = folder_out + "/" + base_filename;
    db_img_path_out = "/" + base_folder + "/" + farm_num + "/" + date_str + "/" + subfolder + "/" + base_filename;
    cout << "[DEBUG] 저장할 파일 경로: " << full_path_out << endl;

}
// 이미지파일 경로를주고 데이터로 받기
std::vector<unsigned char> read_image_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);  // 바이너리 모드로 열기
    if (!file) {
        throw std::runtime_error("파일 열기 실패: " + filename);
    }

    // 파일 끝까지 이동해서 크기 구함
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 벡터 크기 지정 후 읽기
    std::vector<unsigned char> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    return buffer;
}
// 파이썬 이미지 전송
void send_image_to_python(int client_fd, const std::string& full_path, const std::string& filename_name, uint64_t fileSize, int farm_num, int img_num) {
    // 먼저 프로토콜 JSON 구성
    json protocol_json = {
        {"protocol", "img_py"},
        {"img_num", img_num},
        {"farm_num", farm_num},
        {"file_size", fileSize}
    };
    string json_str = protocol_json.dump();

    // 길이 + JSON 문자열 전송
    int32_t len = htonl(json_str.size());
    send(client_fd, &len, sizeof(len), 0);
    send(client_fd, json_str.c_str(), json_str.size(), 0);

    // 이미지 데이터 전송
    ifstream input(full_path, ios::binary);
    if (!input) {
        cerr << "[ERROR] 전송할 파일 열기 실패\n";
        return;
    }

    char buffer[4096];
    while (!input.eof()) {
        input.read(buffer, sizeof(buffer));
        send(client_fd, buffer, input.gcount(), 0);
    }

    input.close();
    cout << "[INFO] Python 클라이언트로 이미지 전송 완료\n";
}
// Base64로 디코딩하기~ 
std::string base64_encode(const std::vector<unsigned char>& data) {
    std::string encoded;
    int val = 0, valb = -6;

    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6)
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);

    while (encoded.size() % 4)
        encoded.push_back('=');

    return encoded;
}

// Base64로 인코딩된 문자열을 받아서, 원래의 바이너리 데이터로 복원
vector<unsigned char> base64_decode(const string& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    vector<unsigned char> ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;

            char_array_3[0] = ((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4)) & 0xff;
            char_array_3[1] = (((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2)) & 0xff;
            char_array_3[2] = (((char_array_4[2] & 0x3) << 6) + char_array_4[3]) & 0xff;

            for (i = 0; i < 3; i++) ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;

        char_array_3[0] = ((char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4)) & 0xff;
        char_array_3[1] = (((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2)) & 0xff;
        char_array_3[2] = (((char_array_4[2] & 0x3) << 6) + char_array_4[3]) & 0xff;

        for (j = 0; j < (i - 1); j++) ret.push_back(char_array_3[j]);
    }
    

    return ret;
}
// Python에서 이미지와 결과 받아 저장하는 함수
void process_python_result_image(const json& received) {
    if (received["protocol"] != "img_py") {
        cerr << "[ERROR] 잘못된 프로토콜\n";
        return;
    }

    int red = received["count"]["red"];
    int yellow = received["count"]["yellow"];
    int green = received["count"]["green"];
    int total_count = red + yellow + green;

    string base64_str = received["result_image_base64"];
    vector<unsigned char> image_data = base64_decode(base64_str);
    cout << "[DEBUG] base64 길이: " << base64_str.length() << endl;
    cout << "[DEBUG] 디코딩된 이미지 크기: " << image_data.size() << " bytes" << endl;

    auto now = chrono::system_clock::now();
    time_t now_c = chrono::system_clock::to_time_t(now);
    tm* now_tm = localtime(&now_c);

    char time_buf[10], date_buf[11];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", now_tm);
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", now_tm);
    string date_str = date_buf;

    ostringstream filename_stream;
    filename_stream << "result_" << (now_tm->tm_year + 1900)
                    << setw(2) << setfill('0') << (now_tm->tm_mon + 1)
                    << setw(2) << setfill('0') << now_tm->tm_mday << "_"
                    << setw(2) << setfill('0') << now_tm->tm_hour
                    << setw(2) << setfill('0') << now_tm->tm_min
                    << setw(2) << setfill('0') << now_tm->tm_sec << ".jpg";
    string result_filename = filename_stream.str();

    string folder, full_path, db_img_path;
    string farm_num = to_string(received["farm_num"]);

    get_image_save_paths("images", farm_num, date_str, "after", result_filename, folder, full_path, db_img_path);

    ofstream ofs(full_path, ios::binary);
    if (!ofs.is_open()) {
        cerr << "[ERROR] 파일 저장 실패: " << full_path << "\n";
        return;
    }

    ofs.write(reinterpret_cast<char*>(image_data.data()), image_data.size());
    ofs.close();
    cout << "[INFO] 이미지 저장 완료: " << full_path << "\n";

    int img_num = received["img_num"];
    if (img_num != -1) {
        update_result_by_img_num(img_num, full_path, total_count, red, yellow, green);
    }
}

// DB INSERT 함수
bool insert_result_to_db(const string& img_path, const string& time_str,
                         int total, int red, int yellow, int green) {
    MYSQL* conn = mysql_init(nullptr);
    if (!mysql_real_connect(conn, host, user, password, dbname, port, nullptr, 0)) {
        cerr << "[ERROR] DB 연결 실패\n";
        return false;
    }

    string query = R"(
        INSERT INTO FARM_IMG (IMG_PATH, TIME, TOTAL_COUNT,
                              RED_TOMATO_COUNT, YELLOW_TOMATO_COUNT, GREEN_TOMATO_COUNT)
        VALUES (?, ?, ?, ?, ?, ?)
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
        cerr << "[ERROR] 쿼리 준비 실패: " << mysql_error(conn) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return false;
    }

    MYSQL_BIND bind[6] = {};
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)img_path.c_str();
    bind[0].buffer_length = img_path.size();

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void*)time_str.c_str();
    bind[1].buffer_length = time_str.size();

    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = (void*)&total;

    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = (void*)&red;

    bind[4].buffer_type = MYSQL_TYPE_LONG;
    bind[4].buffer = (void*)&yellow;

    bind[5].buffer_type = MYSQL_TYPE_LONG;
    bind[5].buffer = (void*)&green;

    if (mysql_stmt_bind_param(stmt, bind) || mysql_stmt_execute(stmt)) {
        cerr << "[ERROR] 쿼리 실행 실패: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt); mysql_close(conn); return false;
    }

    mysql_stmt_close(stmt);
    mysql_close(conn);
    return true;
}

// 큐티 클라이언트 정보 요청
void send_result(json receive, int client_fd){

    string farm_num = receive["farm_num"];
    string date = receive["date"];

    MYSQL* conn = mysql_init(nullptr);
    if (!mysql_real_connect(conn, host, user, password, dbname, port, nullptr, 0)) {
        cerr << "DB 연결 실패: " << mysql_error(conn) << endl;
        ordered_json fail = {
            {"protocol", "select_result_1"},
            {"message", "DB 연결 실패"}
        };
        send_json_response(client_fd, fail.dump());  // 응답 전송
        return;
    }
    // R? - 멀티라인 문자열 리터럴 (raw string literal) 은 여러 줄 문자열을 쉽게 작성할 수 있는 기능
    string query = R"(
        SELECT IMG_PATH,
            TOTAL_COUNT,
            RED_TOMATO_COUNT,
            YELLOW_TOMATO_COUNT,
            GREEN_TOMATO_COUNT,
            IMG_PATH_ASK
        FROM FARM_IMG
        WHERE FARM_NUM = ?
        AND DATE(TIME) = ?
    )";

    // stmt? - Prepared Statement 객체 - struct 타입으로, MySQL C API에서 제공하는 구조체
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        ordered_json fail = {
            {"protocol", "select_result_1"},
            {"message", "쿼리 준비 실패"}
        };
        send_json_response(client_fd, fail.dump());
        mysql_stmt_close(stmt); mysql_close(conn); return;
    }
    // 바인드 - 구조체 세팅
    MYSQL_BIND bind[2] = {};
    int farm_num_ = stoi(farm_num);
    cout << farm_num_ << endl;
    cout << farm_num << endl;
    cout << date << endl;
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &farm_num_;

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void*)date.c_str();
    bind[1].buffer_length = date.length();

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        ordered_json fail = {
            {"protocol", "select_result_1"},
            {"message", "파라미터 바인딩 실패"}
        };
        send_json_response(client_fd, fail.dump());
        mysql_stmt_close(stmt); mysql_close(conn); return;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        ordered_json fail = {
            {"protocol", "select_result_1"},
            {"message", "쿼리 실행 실패"}
        };
        send_json_response(client_fd, fail.dump());
        mysql_stmt_close(stmt); mysql_close(conn); return;
    }
    std::ostringstream debug_query;
    debug_query << "SELECT IMG_PATH, TOTAL_COUNT, RED_TOMATO_COUNT, YELLOW_TOMATO_COUNT, GREEN_TOMATO_COUNT, IMG_PATH_ASK "
                << "FROM FARM_IMG WHERE FARM_NUM = " << farm_num_ << " AND DATE(TIME) = '" << date << "'";

    std::cout << "[DEBUG] 실행될 쿼리:\n" << debug_query.str() << std::endl;
    // 결과 처리
    MYSQL_RES* res = mysql_stmt_result_metadata(stmt);
    MYSQL_BIND result[6] = {};
    char img_path[256], img_path_ask[256];
    int total, red, yellow, green;

    result[0].buffer_type = MYSQL_TYPE_STRING;
    result[0].buffer = img_path;
    result[0].buffer_length = sizeof(img_path);
    result[1].buffer_type = MYSQL_TYPE_LONG;
    result[1].buffer = &total;
    result[2].buffer_type = MYSQL_TYPE_LONG;
    result[2].buffer = &red;
    result[3].buffer_type = MYSQL_TYPE_LONG;
    result[3].buffer = &yellow;
    result[4].buffer_type = MYSQL_TYPE_LONG;
    result[4].buffer = &green;
    result[5].buffer_type = MYSQL_TYPE_STRING;
    result[5].buffer = img_path_ask;
    result[5].buffer_length = sizeof(img_path_ask);

    mysql_stmt_bind_result(stmt, result);

    ordered_json results = {
        {"protocol", "select_result_0"},
        {"results", json::array()},
        {"results_total", json::array()}
    };

    while (mysql_stmt_fetch(stmt) == 0) {
        cout << "반복문1 들어옴?" << endl;
        // char형 이미지 경로를 스트링으로 변환
        string img_path_s = img_path;
        // 앞에 . 을 붙여서 절대경로 완성
        string file_path = "./" + img_path_s;
        // 만든 경로를 함수에 보내서 이미지파일을 vector<unsigned char> 자료형으로 리턴받음 (바이너리)
        vector<unsigned char> tmp = this->read_image_file(file_path);
        // 만든 바이너리 >> vector<unsigned char> 자료형 아이를 스트링으로 변환
        string before = this->base64_encode(tmp);
            cout << "비포 이미지 사진 크기 : " << before.size() << endl;

        string img_path_a = img_path_ask;
        string file_path_a = "./" + img_path_a;
        vector<unsigned char> tmp_a = this->read_image_file(file_path_a);
        string after = this->base64_encode(tmp_a);    
                    cout << "애푸터 이미지 사진 크기 : " << after.size() << endl;

        ordered_json row = {
            {"before", before},
            {"total", total},
            {"red", red},
            {"yellow", yellow},
            {"green", green},
            {"after", after}
        };
        results["results"].push_back(row);
    }

    mysql_free_result(res);
    // mysql_close(conn);    
    query.clear();
    mysql_stmt_close(stmt);
    memset(result, 0, sizeof(result));

  // R? - 멀티라인 문자열 리터럴 (raw string literal) 은 여러 줄 문자열을 쉽게 작성할 수 있는 기능
    query = R"(SELECT TOTAL_COUNT, 
    RED_TOMATO_COUNT , 
    YELLOW_TOMATO_COUNT , 
    GREEN_TOMATO_COUNT, 
    HARVEST_DATE, DATE, GROWTHRATE
    FROM FARM_SUMMARY WHERE 
    FARM_NUM = ?
    )";

 // stmt? - Prepared Statement 객체 - struct 타입으로, MySQL C API에서 제공하는 구조체
    stmt = mysql_stmt_init(conn);
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
        ordered_json fail = {
            {"protocol", "select_result_1"},
            {"message", "쿼리 준비 실패"}
        };
        send_json_response(client_fd, fail.dump());
        mysql_stmt_close(stmt); mysql_close(conn); return;
    }
    // 바인드 - 구조체 세팅 위에서 했으니 생략
    MYSQL_BIND bind_sql[1] = {};

    cout << farm_num_ << endl;
    cout << farm_num << endl;
    cout << date << endl;
    bind_sql[0].buffer_type = MYSQL_TYPE_LONG;
    bind_sql[0].buffer = &farm_num_;


    if (mysql_stmt_bind_param(stmt, bind_sql) != 0) {
        ordered_json fail = {
            {"protocol", "select_result_1"},
            {"message", "샌드뤼졀트 둡너째 파라미터 바인딩 실패"}
        };
        send_json_response(client_fd, fail.dump());
        mysql_stmt_close(stmt); mysql_close(conn); return;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        ordered_json fail = {
            {"protocol", "select_result_1"},
            {"message", "샌드뤼졀트 둡너째 파라미터 바인딩 실패"}
        };
        send_json_response(client_fd, fail.dump());
        mysql_stmt_close(stmt); mysql_close(conn); return;
    }

    // 결과 처리
            cout <<"d여기!??   5 "  << endl;

    res = mysql_stmt_result_metadata(stmt);
            cout <<"d여기!??   6 "  << endl;

    MYSQL_BIND result_[7] = {};
            cout <<"d여기!??   7 "  << endl;

    char HARVEST_DATE[100], DATE[100];
    int TOTAL_COUNT, RED_TOMATO_COUNT, YELLOW_TOMATO_COUNT, GREEN_TOMATO_COUNT, GROWTHRATE;
            cout <<"d여기!??   8 "  << endl;

    result_[0].buffer_type = MYSQL_TYPE_LONG;
    result_[0].buffer = &TOTAL_COUNT;
    result_[1].buffer_type = MYSQL_TYPE_LONG;
    result_[1].buffer = &RED_TOMATO_COUNT;
    result_[2].buffer_type = MYSQL_TYPE_LONG;
    result_[2].buffer = &YELLOW_TOMATO_COUNT;
    result_[3].buffer_type = MYSQL_TYPE_LONG;
    result_[3].buffer = &GREEN_TOMATO_COUNT;
    result_[4].buffer_type = MYSQL_TYPE_STRING;
    result_[4].buffer = &HARVEST_DATE;
    result_[4].buffer_length = sizeof(HARVEST_DATE);
    result_[5].buffer_type = MYSQL_TYPE_STRING;
    result_[5].buffer = &DATE;
    result_[5].buffer_length = sizeof(DATE);
    result_[6].buffer_type = MYSQL_TYPE_LONG;
    result_[6].buffer = &GROWTHRATE;
    result_[6].buffer_length = sizeof(GROWTHRATE);

    mysql_stmt_bind_result(stmt, result_);

            cout <<"d여기!??   10 "  << endl;

    while (mysql_stmt_fetch(stmt) == 0) {
            cout <<"d여기!??   11" << endl;

        cout << total << "  " << red<< endl;
        ordered_json row = {
            {"TOTAL_COUNT", TOTAL_COUNT},
            {"RED_TOMATO_COUNT", RED_TOMATO_COUNT},
            {"YELLOW_TOMATO_COUNT", YELLOW_TOMATO_COUNT},
            {"GREEN_TOMATO_COUNT", GREEN_TOMATO_COUNT},
            {"HARVEST_DATE", HARVEST_DATE},
            {"DATE", DATE},
            {"GROWTHRATE", GROWTHRATE}
        };
        results["results_total"].push_back(row);
    }

    send_json_response(client_fd, results.dump());
        
    mysql_free_result(res);
    mysql_stmt_close(stmt);
    mysql_close(conn);    
}

};

// 로그인 체크 함수
int check_login(int client_fd, handle_class pipi)  {

    string msg;
    // --- JSON 길이 수신 ---
    int32_t jsonLenNet = 0;
    int readLen = read(client_fd, &jsonLenNet, sizeof(jsonLenNet));
    if (readLen <= 0) return 0;

    int jsonLen = ntohl(jsonLenNet);
    vector<char> buf(jsonLen);
    int total = 0;

    while (total < jsonLen) {
        int r = read(client_fd, buf.data() + total, jsonLen - total);
        if (r <= 0) break;
        total += r;
    }

    string json_str(buf.begin(), buf.end());
    cout << "받은 JSON: " << json_str << "\n";

    json received = json::parse(json_str);
    string protocol = received["protocol"];

    if (protocol == "login") {
        msg = pipi.handling_login(json_str);
        pipi.send_json_response(client_fd, msg);
        msg.clear();
        return 1;
    } else if (protocol == "LOGIN_PY") {
        // 파이썬 클라이언트 연결 처리
        python_fd = client_fd;
        cout << "[INFO] Python 클라이언트 등록 완료 (fd=" << python_fd << ")\n";
        return 2;
    } else return 0;
}
// qt 클라이언트 요청 컨트롤
void control_clt(int client_fd, handle_class pipi) { 

    // --- JSON 길이 수신 ---
    int32_t jsonLenNet = 0;
    int readLen = read(client_fd, &jsonLenNet, sizeof(jsonLenNet));
    if (readLen <= 0) return;

    int jsonLen = ntohl(jsonLenNet);
    vector<char> buf(jsonLen);
    int total = 0;

    while (total < jsonLen) {
        int r = read(client_fd, buf.data() + total, jsonLen - total);
        if (r <= 0) break;
        total += r;
    }

    string json_str(buf.begin(), buf.end());
    cout << "받은 JSON: " << json_str << "\n";

    json received = json::parse(json_str);
    string protocol = received["protocol"];

    if (protocol == "image") {
        cout << "프로토콜 이미지에 들어왔어요\n";
        pipi.receiving_image(client_fd, received);
    }
    else if(protocol == "select_result"){
        cout << "프로토콜 셀렉리절트 들어왔어요\n";
        pipi.send_result(received, client_fd);
    }
}

// 파이썬 요청 컨트롤
void control_py(int client_fd, handle_class pipi) {

    // --- JSON 길이 수신 ---
    int32_t jsonLenNet = 0;
    int readLen = read(client_fd, &jsonLenNet, sizeof(jsonLenNet));
    if (readLen <= 0) return;

    int jsonLen = ntohl(jsonLenNet);
    vector<char> buf(jsonLen);
    int total = 0;

    while (total < jsonLen) {
        int r = read(client_fd, buf.data() + total, jsonLen - total);
        if (r <= 0) break;
        total += r;
    }

    string json_str(buf.begin(), buf.end());
    cout << "받은 JSON: " << json_str << "\n";

    json received = json::parse(json_str);
    string protocol = received["protocol"];

    if(protocol == "img_py") {

        cout << "프로토콜 센드피와이 들어왔어요" << "\n";  

        pipi.process_python_result_image(received);


    } 
}
// 스레드 생성 함수
void pipipi(int client_fd)
{
    // send_json 으로 무조건 4바이트 길이, 내용을 보내줌
    cout << "[INFO] 클라이언트 연결 성공 pipipi 들어왔어요~\n";
    handle_class pipi;

    // 로그인 체크하는 함수
    int who = check_login(client_fd, pipi);

    while(who == 1) {
        control_clt(client_fd, pipi);
    }
    while(who == 2) {
        control_py(client_fd, pipi);
    }

    close(client_fd);
    cout << "클라이언트 종료~" << endl;
}

class server {
    private:
        int server_fd;
        sockaddr_in addr{};

    public:
        server() {
            cout << "[DEBUG] 서버 소켓 생성 시도...\n";

            // 서버 소켓 생성 (클라이언트 접속을 기다릴 소켓)
            server_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (server_fd < 0) {
                cerr << "[ERROR] 서버 소켓 생성 실패\n";
                return;
            }
            // 소켓 - 컴퓨터끼리 네트워크를 통해 데이터를 주고 받기 위한 문

            cout << "[DEBUG] 포트 재사용 옵션 설정\n";
            // 포트 재사용 옵션 설정 (서버 재시작 시 'address already in use' 방지)
            int opt = 1;
            if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                // SO_REUSEADDR - 이미 사용 중인 주소/포트를 재사용 가능하게 함 // 커널이 일정 시간 동안 그 포트를 TIME_WAIT 상태로 보류 하자나!
                cerr << "[ERROR] setsockopt 실패\n";
                close(server_fd);
                return;
            }

            // 서버 주소 정보 설정
            sockaddr_in addr{};
            this->addr.sin_family = AF_INET;                  // IPv4 사용
            this->addr.sin_port = htons(CLIENT_PORT);         // 수신 포트 설정
            this->addr.sin_addr.s_addr = INADDR_ANY;          // 모든 IP로부터의 접속 허용

        }
        void open_server() {
            cout << "[DEBUG] 바인딩 시도...\n";
            // 서버 소켓에 주소 정보 바인딩
            if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
                cerr << "[ERROR] 바인딩 실패\n";
                close(server_fd);
                return;
            }

            cout << "[DEBUG] 클라이언트 수신 대기 시작...\n";
            // 연결 대기 시작 (큐 길이 1)
            if (listen(server_fd, 1) < 0) {
                cerr << "[ERROR] listen 실패\n";
                close(server_fd);
                return;
            }

            cout << "[INFO] 클라이언트 연결 대기 중 (포트 " << CLIENT_PORT << ")...\n";

            while (true) 
            {
                cout << "클라이언트 연결 대기하나?      "  << endl;
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_socket = accept(server_fd, (sockaddr*)&client_addr, &client_len);
                // int client_socket = accept(server_fd, nullptr, nullptr);
                if (client_socket < 0) {
                    perror("accept 실패");    close(server_fd);
                    continue;
                }

                cout << "클라이언트 연결됨: "
                << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port) << "\n";

                // 여려명의 클라이언트를 받으려면 스레드로 돌려요 
                thread t(pipipi, client_socket);
                // 방금 만든 스레드를 방치하기~
                t.detach(); // 독립 실행
            }
        }
};


// 메인 함수: 클라이언트 이미지 수신 + 저장 + 전송
int main() {

    server server01;
    server01.open_server();

    return 0;
}