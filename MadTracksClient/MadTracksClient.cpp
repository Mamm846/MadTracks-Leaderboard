#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <fstream>
#include <string>
#include "SupabaseManager.h"
#include "SupabaseConfig.h"
// ==========================================
// CLASSE 1 : GESTION DE LA MEMOIRE
// ==========================================
class MemoryManager {
private:
    DWORD pid = 0;
    HANDLE hProcess = nullptr;
    uintptr_t moduleBase = 0;
    std::wstring processName;

    DWORD GetProcessIdByName(const wchar_t* name) {
        DWORD foundPid = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe{ sizeof(pe) };
            if (Process32FirstW(snapshot, &pe)) {
                do {
                    if (_wcsicmp(pe.szExeFile, name) == 0) {
                        foundPid = pe.th32ProcessID;
                        break;
                    }
                } while (Process32NextW(snapshot, &pe));
            }
            CloseHandle(snapshot);
        }
        return foundPid;
    }

    uintptr_t GetModuleBaseAddress(DWORD targetPid, const wchar_t* name) {
        uintptr_t base = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, targetPid);
        if (snapshot != INVALID_HANDLE_VALUE) {
            MODULEENTRY32W me{ sizeof(me) };
            if (Module32FirstW(snapshot, &me)) {
                do {
                    if (_wcsicmp(me.szModule, name) == 0) {
                        base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                        break;
                    }
                } while (Module32NextW(snapshot, &me));
            }
            CloseHandle(snapshot);
        }
        return base;
    }

public:
    MemoryManager(const std::wstring& procName) : processName(procName) {}
    ~MemoryManager() { if (hProcess) CloseHandle(hProcess); }

    bool Attacher() {
        pid = GetProcessIdByName(processName.c_str());
        if (!pid) return false;

        hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) return false;

        moduleBase = GetModuleBaseAddress(pid, processName.c_str());
        return moduleBase != 0;
    }

    uintptr_t GetModuleBase() const { return moduleBase; }

    template <typename T>
    T Read(uintptr_t address) {
        T value{};
        ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr);
        return value;
    }
};

// ==========================================
// CLASSE 2 : GESTION DES RECORDS (SAUVEGARDE)
// ==========================================
class RecordManager {
private:
    std::string filename;
    std::map<int, double> records;

public:
    RecordManager(const std::string& file) : filename(file) {
        Charger();
    }

    void Charger() {
        std::ifstream file(filename);
        int trackID;
        double time;
        while (file >> trackID >> time) {
            records[trackID] = time;
        }
    }

    void Sauvegarder() const {
        std::ofstream file(filename);
        for (const auto& [track, time] : records) {
            file << track << " " << time << "\n";
        }
    }

    // Retourne true si c'est un nouveau record
    bool SoumettreTemps(int trackID, double time) {
        if (!records.count(trackID) || time < records[trackID]) {
            records[trackID] = time;
            Sauvegarder();
            return true;
        }
        return false;
    }

    double GetRecord(int trackID) {
        return records.count(trackID) ? records[trackID] : 0.0;
    }
};

// ==========================================
// CLASSE 3 : LE TRACKER (MACHINE A ETATS)
// ==========================================
class MadTracksTracker {
private:
    MemoryManager mem;
    RecordManager records;
    std::string pseudo;
    SupabaseManager& web;
    // Offsets
    const uintptr_t OFFSET_BASE_SESSION = 0x28D9F8;
    const uintptr_t OFFSET_TRACK_ID = 0x1468;
    const uintptr_t OFFSET_TIMER = 0x17E8;
    const uintptr_t OFFSET_LAP = 0x17F8;
    const uintptr_t OFFSET_FINISHED = 0x1800; // Drapeau de fin de course

    // Dictionnaire des circuits
    std::map<int, std::string> nomsCircuits = {
        { 210, "Training day" },
        { 275, "Black out" },
        { 338, "Heavy croquet" },
        { 339, "Bob's leg" },
        { 397, "Fast food" },
        { 449, "Toygara falls" },
        { 466, "Rock and race" },
        { 497, "The persuaders" },
        { 501, "Well's bell" },   // Circuit de survie : mort comptabilisée comme fin de course (pas de record de vitesse)
        { 512, "From the sky" },
        { 519, "Stronghold" },
        { 551, "Thin line" },
        { 567, "Titanic" },       // Circuit de survie : mort comptabilisée comme fin de course (pas de record de vitesse)
        { 593, "Ice cream" },
        { 628, "Time attack" },
        { 636, "Speed racer" },
        { 700, "Tea time" },
        { 706, "Mona racer" },
        { 736, "Half pipe" },
        { 820, "Toy race" },
        { 830, "My way" }
    };
    // Etats de la machine
    enum class Etat { RECHERCHE_JEU, DANS_MENU, EN_COURSE, COURSE_TERMINEE };
    Etat etatActuel = Etat::RECHERCHE_JEU;

public:
    MadTracksTracker(std::string p, SupabaseManager& w)
        : mem(L"MadTracks.exe"), records("records.txt"), pseudo(p), web(w) {
    }

    void Run() {
        std::cout << "--- Mad Tracks Auto-Tracker Initie ---\n";
        std::cout << "Appuie sur [FIN] pour quitter.\n\n";

        while (true) {
            if (GetAsyncKeyState(VK_END) & 1) break;

            MettreAJour();
            Sleep(33); // ~30 FPS
        }
    }

private:
    void MettreAJour() {
        if (etatActuel == Etat::RECHERCHE_JEU) {
            if (mem.Attacher()) {
                etatActuel = Etat::DANS_MENU;
                std::cout << "Jeu trouve. En attente d'une course...\n";
            }
            else {
                std::cout << "\rEn attente de MadTracks.exe...     " << std::flush;
                Sleep(500);
            }
            return;
        }

        uintptr_t sessionPtr = mem.GetModuleBase() + OFFSET_BASE_SESSION;
        uintptr_t sessionBase = mem.Read<uintptr_t>(sessionPtr);

        if (sessionBase == 0) {
            if (etatActuel != Etat::DANS_MENU) {
                etatActuel = Etat::DANS_MENU;
                std::cout << "\rRetour au menu principal...                                \n" << std::flush;
            }
            return;
        }

        // Lecture des variables de la course
        int trackID = mem.Read<int>(sessionBase + OFFSET_TRACK_ID);
        double chrono = mem.Read<double>(sessionBase + OFFSET_TIMER);
        int lap = mem.Read<int>(sessionBase + OFFSET_LAP);
        int finished = mem.Read<int>(sessionBase + OFFSET_FINISHED);

        // MACHINE A ETATS
        if (chrono > 0.1 && finished != 1 && etatActuel != Etat::EN_COURSE) {
            etatActuel = Etat::EN_COURSE;
            std::cout << "\n>>> DEBUT DE COURSE (Track ID: " << trackID << ") <<<\n";
        }
        else if (chrono < 0.05 && etatActuel != Etat::DANS_MENU) {
            etatActuel = Etat::DANS_MENU;
            std::cout << "\n[Chrono a zero / En attente du depart...]\n";
        }
        else if (chrono < 0.1 && etatActuel == Etat::EN_COURSE) {
            etatActuel = Etat::DANS_MENU;
            std::cout << "\n[Course annulee / Retour au menu]\n";
        }

        if (etatActuel == Etat::EN_COURSE) {
            if (finished == 1) {
                // DECLENCHEUR : LA COURSE VIENT DE SE TERMINER
                bool estCircuitSurvie = (trackID == 501 || trackID == 567);
				if (estCircuitSurvie && lap == 0) { // Condition à modifier pour reconnaitre la mort sur les circuits de survie
                    etatActuel = Etat::COURSE_TERMINEE;
                    std::cout << "\n\n=== COURSE TERMINEE (Circuit de survie) ===";
					std::cout << "\nVous êtes mort.\n\n";
				}
                else {
                    etatActuel = Etat::COURSE_TERMINEE;
                    std::cout << "\n\n=== LIGNE D'ARRIVEE FRANCHIE ===";
                    std::cout << "\nTemps final : " << std::fixed << std::setprecision(3) << chrono << " sec";

                    if (records.SoumettreTemps(trackID, chrono)) {
                        std::cout << "\n*** NOUVEAU RECORD PERSONNEL ! ***\n";

                        // ENVOI AU SERVEUR
                        std::cout << "Envoi au serveur Supabase... ";
                        if (web.EnvoyerRecord(pseudo, trackID, chrono)) {
                            std::cout << "[OK]\n\n";
                        }
                        else {
                            std::cout << "[ECHEC]\n\n";
                        }
                    }
                    else {
                        std::cout << "\nRecord actuel : " << records.GetRecord(trackID) << " sec\n\n";
                    }
                }
            }
            else {
                // Affichage en temps réel
                int min = static_cast<int>(chrono) / 60;
                double sec = chrono - (min * 60);
                std::cout << "\r[Tour " << lap << "] Chrono: " << min << ":"
                    << std::setfill('0') << std::setw(2) << static_cast<int>(sec)
                    << "." << std::setw(3) << static_cast<int>((sec - static_cast<int>(sec)) * 1000)
                    << "    " << std::flush;
            }
        }
    }
};

// ==========================================
// POINT D'ENTREE
// ==========================================
int main() {
    std::string pseudo = "Anonyme";
    std::ifstream fichierJoueur("joueur.txt");

    if (fichierJoueur.is_open()) {
        std::getline(fichierJoueur, pseudo);
        fichierJoueur.close();
        std::cout << "Connecte en tant que : " << pseudo << "\n";
    }
    else {
        std::cout << "[ATTENTION] Fichier joueur.txt introuvable. Vous jouez en Anonyme.\n";
    }

    // On initialise le gestionnaire web avec tes cles
    // /!\ ATTENTION: Retire le "https://" de ton URL Supabase ici !
    SupabaseManager web(SUPABASE_HOST, SUPABASE_ANON_KEY);
    // Lance ton tracker en lui passant le pseudo et le gestionnaire web
    MadTracksTracker tracker(pseudo, web);
    tracker.Run();

    return 0;
}
