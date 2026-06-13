#include <windows.h>
#include <string>
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

// Menü ID tanımlamaları
#define ID_MENU_SORUSOR 1001
#define ID_MENU_TEMIZLE 1002
#define ID_MENU_CIKIS    1003
#define ID_MENU_HAKKINDA 1004
#define ID_BUTON_GONDER  1005

// Global Bileşenler
HWND hEditOutput, hEditInput, hBtnGonder;
// Verilen Groq API Key doğrudan koda entegre edildi
string apiKey = "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"; 

// Curl için callback
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Groq API İstek Fonksiyonu
string AskGroq(const string& prompt) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        string auth_header = "Authorization: Bearer " + apiKey;
        headers = curl_slist_append(headers, auth_header.c_str());

        json payload;
        payload["model"] = "llama-3.3-70b-versatile";
        payload["messages"] = json::array({{{"role", "user"}, {"content", prompt}}});
        string json_str = payload.dump();

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.groq.com/openai/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res != CURLE_OK) return "Hata: API baglantisi basarisiz.";

        try {
            auto res_json = json::parse(readBuffer);
            if (res_json.contains("choices") && !res_json["choices"].empty()) {
                return res_json["choices"][0]["message"]["content"];
            } else if (res_json.contains("error")) {
                return "API Hatasi: " + res_json["error"]["message"].get<string>();
            }
        } catch (...) {
            return "Yanit cozumlenirken bir hata olustu.";
        }
    }
    return "Bir hata olustu veya API sunucusuna ulasilamadi.";
}

// Klasik Windows Menü Çubuğunu Oluşturan Fonksiyon
void EkleMenu(HWND hwnd) {
    HMENU hMenuBar = CreateMenu();
    HMENU hMenuDosya = CreateMenu();
    HMENU hMenuYardim = CreateMenu();

    // Üst Menü elemanları (IDA Pro stili yerleşim)
    AppendMenuW(hMenuDosya, MF_STRING, ID_MENU_SORUSOR, L"Yapay Zekaya Odaklan");
    AppendMenuW(hMenuDosya, MF_STRING, ID_MENU_TEMIZLE, L"Ekrani Temizle");
    AppendMenuW(hMenuDosya, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenuDosya, MF_STRING, ID_MENU_CIKIS, L"Cikis");

    AppendMenuW(hMenuYardim, MF_STRING, ID_MENU_HAKKINDA, L"Uygulama Hakkinda");

    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hMenuDosya, L"Yapay Zeka (AI)");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hMenuYardim, L"Yardim");

    SetMenu(hwnd, hMenuBar);
}

// Window Procedure (Mesaj Döngüsü Yönetimi)
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: {
            EkleMenu(hwnd);

            // Çıktı Geçmiş Alanı (Klasik Salt Okunur Edit Kontrolü)
            hEditOutput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"--- AI-IDA Professional Enterprise ---\r\nModel: Llama 3.3 Versatile (Groq API)\r\nDurum: Baglanti Hazir.\r\n\r\n", 
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                10, 10, 560, 300, hwnd, NULL, NULL, NULL);

            // Kullanıcı Mesaj Giriş Alanı
            hEditInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", 
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                10, 320, 450, 25, hwnd, NULL, NULL, NULL);

            // Gönder Butonu
            hBtnGonder = CreateWindowW(L"BUTTON", L"Gonder", 
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                470, 320, 100, 25, hwnd, (HMENU)ID_BUTON_GONDER, NULL, NULL);
            
            break;
        }
        case WM_COMMAND: {
            switch(LOWORD(wParam)) {
                case ID_MENU_SORUSOR:
                    SetFocus(hEditInput);
                    break;
                case ID_MENU_TEMIZLE:
                    // Çıktı ekranını sıfırla
                    SetWindowTextW(hEditOutput, L"--- Ekran Temizlendi ---\r\n\r\n");
                    break;
                case ID_MENU_CIKIS:
                    DestroyWindow(hwnd);
                    break;
                case ID_MENU_HAKKINDA:
                    MessageBoxW(hwnd, L"AI-IDA v1.0 Professional\n\nGroq API ve yerel Win32 API kullanilarak hazirlanmistir.\nModel: Llama 3.3 Versatile", L"Hakkinda", MB_OK | MB_ICONINFORMATION);
                    break;
                case ID_BUTON_GONDER: {
                    wchar_t inputBuf[1024];
                    GetWindowTextW(hEditInput, inputBuf, 1024);
                    
                    if (wcslen(inputBuf) > 0) {
                        // Kullanıcı girdisini ekrana yazdır
                        int len = GetWindowTextLengthW(hEditOutput);
                        SendMessageW(hEditOutput, EM_SETSEL, len, len);
                        SendMessageW(hEditOutput, EM_REPLACESEL, FALSE, (LPARAM)L"Siz: ");
                        SendMessageW(hEditOutput, EM_REPLACESEL, FALSE, (LPARAM)inputBuf);
                        SendMessageW(hEditOutput, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n\r\n[..] Groq Sunucusundan yanit bekleniyor...\r\n");

                        // UTF-16'dan UTF-8'e dönüştür (API için)
                        wstring ws(inputBuf);
                        string strPrompt(ws.begin(), ws.end());
                        
                        // Yapay zekaya sor
                        string aiResponse = AskGroq(strPrompt);
                        wstring wsResponse(aiResponse.begin(), aiResponse.end());

                        // Bekleniyor yazısını kaldır/üzerine yaz veya yeni satıra ekle
                        len = GetWindowTextLengthW(hEditOutput);
                        SendMessageW(hEditOutput, EM_SETSEL, len, len);
                        SendMessageW(hEditOutput, EM_REPLACESEL, FALSE, (LPARAM)L"AI-IDA: ");
                        SendMessageW(hEditOutput, EM_REPLACESEL, FALSE, (LPARAM)wsResponse.c_str());
                        SendMessageW(hEditOutput, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n\r\n--------------------------------------------------\r\n\r\n");

                        // Input alanını temizle ve odağı orada tut
                        SetWindowTextW(hEditInput, L"");
                        SetFocus(hEditInput);
                    }
                    break;
                }
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Windows Program Giriş Noktası
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    curl_global_init(CURL_GLOBAL_ALL);

    WNDCLASSW wc = {0};
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInstance;
    wc.lpszClassName = L"AI_IDA_CLASS";
    wc.lpfnWndProc = WndProc;

    if(!RegisterClassW(&wc)) return -1;

    // Pencere Boyutları ve Başlığı
    CreateWindowW(L"AI_IDA_CLASS", L"AI-IDA Professional - Llama 3.3 Versatile Edition", 
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 600, 430, NULL, NULL, hInstance, NULL);

    MSG msg = {0};
    while(GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    curl_global_cleanup();
    return 0;
}

