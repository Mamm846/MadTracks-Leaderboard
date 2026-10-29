#pragma once
#include <string>
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#pragma comment(lib, "winhttp.lib") // Demande à Visual Studio de lier l'outil internet de Windows

class SupabaseManager {
private:
    std::wstring host;
    std::wstring apiKey;

    // Convertit le texte classique en texte lisible par Windows
    std::wstring s2ws(const std::string& s) {
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
        wchar_t* buf = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buf, len);
        std::wstring res(buf);
        delete[] buf;
        return res;
    }

public:
    SupabaseManager(std::string urlHost, std::string key) {
        host = s2ws(urlHost);
        apiKey = s2ws(key);
    }

    bool EnvoyerRecord(std::string pseudo, int trackId, double chrono) {
        // Ouvre la connexion
        HINTERNET hSession = WinHttpOpen(L"MadTracks Tracker/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return false;

        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/rest/v1/records", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

        // Prépare le badge d'accès (API Key)
        std::wstring headers = L"Content-Type: application/json\r\napikey: " + apiKey + L"\r\nAuthorization: Bearer " + apiKey + L"\r\nPrefer: return=minimal\r\n";

        // Prépare le paquet de données (JSON)
        std::string jsonStr = "{\"player_name\":\"" + pseudo + "\",\"track_id\":" + std::to_string(trackId) + ",\"time\":" + std::to_string(chrono) + "}";

        // Envoie la requête
        BOOL bResults = WinHttpSendRequest(hRequest, headers.c_str(), -1, (LPVOID)jsonStr.c_str(), jsonStr.length(), jsonStr.length(), 0);
        if (bResults) {
            bResults = WinHttpReceiveResponse(hRequest, NULL);
        }

        // Si ça a planté, on affiche le code d'erreur Windows
        if (!bResults) {
            std::cout << "(Code erreur: " << GetLastError() << ") ";
        }

        // Ferme les portes
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        return bResults == TRUE;
    }
};