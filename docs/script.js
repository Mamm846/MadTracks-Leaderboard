const url = "https://mmzhuycnbmqzcicknzjr.supabase.co/rest/v1/records?select=player_name,track_id,time";
const apiKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im1temh1eWNuYm1xemNpY2tuempyIiwicm9sZSI6ImFub24iLCJpYXQiOjE3OTMyMjI3NzIsImV4cCI6MjEwODc5ODc3Mn0.ZdU-gDN2cpzVKijt8gEBpMETRAguwlCdB5A0ANiZpzE";
const nomsCircuits = {
    210: "Training day", 275: "Black out", 338: "Heavy croquet", 339: "Bob's leg",
    397: "Fast food", 449: "Toygara falls", 466: "Rock and race", 497: "The persuaders",
    501: "Well's bell", 512: "From the sky", 519: "Stronghold", 551: "Thin line",
    567: "Titanic", 593: "Ice cream", 628: "Time attack", 636: "Speed racer",
    700: "Tea time", 706: "Mona racer", 736: "Half pipe", 820: "Toy race", 830: "My way"
};
const selectCircuit = document.getElementById("circuit");
const statut = document.getElementById("statut");
const titreClassement = document.getElementById("titre-classement");
const podium = document.getElementById("podium");
const liste = document.getElementById("classement");
const vide = document.getElementById("vide");
let records = [];

function creerCartePodium(record, position) {
    const carte = document.createElement("article");
    carte.className = `podium-card${position === 0 ? " first" : ""}`;
    carte.innerHTML = `<span class="medal">${["🥇", "🥈", "🥉"][position]}</span>
        <p class="podium-name"></p><p class="podium-time"></p>`;
    carte.querySelector(".podium-name").textContent = record.player_name;
    carte.querySelector(".podium-time").textContent = `${record.time} s`;
    return carte;
}

function afficherClassement(trackId) {
    const classement = records
        .filter(record => String(record.track_id) === String(trackId))
        .sort((a, b) => Number(a.time) - Number(b.time));
    const nomCircuit = nomsCircuits[trackId] || `Circuit ${trackId}`;

    titreClassement.textContent = nomCircuit;
    podium.replaceChildren();
    liste.replaceChildren();
    vide.classList.toggle("hidden", classement.length !== 0);
    classement.slice(0, 3).forEach((record, position) => podium.appendChild(creerCartePodium(record, position)));
    classement.slice(3).forEach((record, index) => {
        const li = document.createElement("li");
        li.className = "row";
        li.innerHTML = `<span class="rank"></span><span class="player"></span><span class="time"></span>`;
        li.querySelector(".rank").textContent = `#${index + 4}`;
        li.querySelector(".player").textContent = record.player_name;
        li.querySelector(".time").textContent = `${record.time} s`;
        liste.appendChild(li);
    });
    statut.textContent = classement.length === 0 ? "Pas encore de record" : `${classement.length} record(s)`;
}

function remplirCircuits() {
    Object.entries(nomsCircuits).forEach(([trackId, nom]) => {
        const option = document.createElement("option");
        option.value = trackId;
        option.textContent = nom;
        selectCircuit.appendChild(option);
    });
    selectCircuit.disabled = false;
    selectCircuit.value = Object.keys(nomsCircuits)[0];
    afficherClassement(selectCircuit.value);
}

async function chargerClassement() {
    if (!apiKey) {
        throw new Error("La clé anon Supabase n'est pas configurée dans frontend/script.js.");
    }
    const response = await fetch(url, { headers: { "apikey": apiKey, "Authorization": ["Bearer", apiKey].join(" ") } });
    if (!response.ok) throw new Error(`Supabase a répondu ${response.status}: ${await response.text()}`);
    const data = await response.json();
    if (!Array.isArray(data)) throw new Error("La réponse Supabase n'est pas une liste de records.");
    records = data;
    remplirCircuits();
}

selectCircuit.addEventListener("change", () => afficherClassement(selectCircuit.value));
chargerClassement().catch(error => {
    console.error("Impossible de charger le classement :", error);
    statut.textContent = `Erreur : ${error.message}`;
});
