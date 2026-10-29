# Mad Tracks Tracker

Tracker de temps et leaderboard pour le jeu **Mad Tracks**.

Le projet contient :

- un client C++ qui détecte les temps réalisés dans le jeu et les envoie vers Supabase ;
- une interface web statique qui affiche les records par circuit ;
- un podium pour les trois meilleurs temps et une liste complète des résultats suivants.

## Structure du projet

```text
frontend/
├── index.html   # Structure de la page
├── style.css    # Interface et responsive design
└── script.js    # Chargement Supabase et affichage du classement

MadTracksClient/
└── ...          # Client C++ et projet Visual Studio
```

## Configuration de Supabase

La clé publique **anon** du frontend est directement définie dans [frontend/script.js](frontend/script.js). Elle est destinée à être exposée au navigateur ; ne jamais y placer une clé `service_role`.

### Client C++

Copier `MadTracksClient/SupabaseConfig.example.h` sous le nom `MadTracksClient/SupabaseConfig.h`, puis renseigner les mêmes valeurs :

```cpp
inline constexpr const char* SUPABASE_HOST = "votre-projet.supabase.co";
inline constexpr const char* SUPABASE_ANON_KEY = "votre_cle_anon_supabase";
```

Ne jamais placer une clé `service_role` dans le frontend. Une clé publique `anon` doit également être protégée par les règles RLS adaptées dans Supabase.

La table utilisée par l'interface est `records` et doit exposer les colonnes suivantes :

| Colonne | Utilisation |
| --- | --- |
| `player_name` | Nom du joueur |
| `track_id` | Identifiant du circuit |
| `time` | Temps réalisé en secondes |

## Lancer le leaderboard

Le frontend est une page statique. Pour éviter les limitations des navigateurs avec `file://`, lancer un serveur local depuis la racine du dépôt :

```powershell
python -m http.server 8000
```

Puis ouvrir :

<http://localhost:8000/frontend/>

Une extension comme **Live Server** dans VS Code peut également être utilisée.

## Fonctionnement du classement

- Le circuit est choisi avec la liste déroulante.
- Les records sont filtrés par `track_id`.
- Les temps sont triés du plus rapide au plus lent.
- Les trois premiers apparaissent sur le podium avec une médaille.
- Tous les records suivants sont affichés dans la liste sous le podium.

## Client C++

Le client C++ se trouve dans `MadTracksClient/` et peut être ouvert avec Visual Studio via le fichier `.slnx`.

Le projet utilise WinHTTP pour envoyer les records vers l'endpoint REST de Supabase. Configurez les informations Supabase dans le code avant de compiler le client, puis compilez la configuration souhaitée depuis Visual Studio.

Les dossiers de compilation, les journaux, les exécutables, les configurations locales et les fichiers utilisateur Visual Studio sont exclus du contrôle de version par [.gitignore](.gitignore).

## Sécurité

- Ne pas publier de clé `service_role`.
- Ne pas commiter de données personnelles ou de fichiers de records locaux.
- Vérifier les politiques RLS Supabase avant de rendre la table accessible publiquement.
