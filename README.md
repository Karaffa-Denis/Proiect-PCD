# Proiect-PCD
T8 - Document contur și perspectiva
Operații avute în vedere (nivel server):
- Conversia în tonuri de gri;
- Aplicarea filtrului threshold pentru a distinge textul de fundal;
- Identificarea și extragerea conturului cel mai mare, presupus a fi documentul; -
Redimensionarea și ajustarea perspectivei documentului

#Tehnologii folosite:
1. C++ pentru server, client, si admin
2. Python pentru un client secundar
3. OPENCV pentru procesarea imaginilor
4. Protocol de trimitere a datelor TCP
5. Comunicatie prin socket-uri INET Client-Server
6. Comunicatie prin socket-uri UNIX Admin-Server


#Structura:
server.cpp: creeaza două thread-uri, inetds si unixds

inetds.cpp: thread INET: accept clienți, procesare OpenCV în thread detached

unixds.cpp: thread UNIX: accept admin, handle_admin() cu comenzi LIST/KICK/SHUTDOWN

inetclient.cpp: client INET: trimite imagine SEND:<cale>, primește imaginea procesată

unixclient.cpp: client admin: TUI CLI pentru comenzi administrative

proto.h / .cpp: Variabile globale: shutdown_in_progress, connected_clients[], remove_client()

#Mod de functionare:
Serverul menține două thread-uri, unul pentru interacțiunile INET cu un număr
mai mare de clienți, și unul UNIX pentru interacțiunile cu un singur client admin. Cele două
au ambele o structură TCP de transfer.
Interfata CLI: atât adminul cât si clienții interacționează cu serverul prin terminal.
Clienții sunt capabili să trimită cereri de procesare a unei imagini, și așteaptă imaginea
trimisă de către server.
Adminul poate vedea o lista a clienților, poate deconecta clienții, poate inchide serverul, și
poate vizualiza log-urile serverului

./inetclient
EXIT -> parasim interfata
SEND:<path> -> trimitem spre inetds o imagine spre procesare

./unixclient
LIST: Afișează fd-urile tuturor clienților conectați curent
KICK:<fd> : Închide socket-ul unui client și îl elimină din listă
SHUTDOWN: Oprește serverul: setează flag-ul shutdown_in_progress
EXIT: Deconectează adminul, permite o nouă conexiune admin


