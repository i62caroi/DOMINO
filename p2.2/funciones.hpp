#ifndef FUNCIONES_HPP
#define FUNCIONES_HPP

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <signal.h>
#include "ficha.hpp"
#include "partida.hpp"
#include "jugador.hpp"

void enviarMensaje(int socket, const char *cadena){
	char buffer[250];
	bzero(buffer,sizeof(buffer));
	sprintf(buffer, "%s",cadena);
	send(socket,buffer,strlen(buffer),0);
}

void setIDPartidaySockets(int &i, std::vector<Partida> &partidas, int &idPartida, int &socket1, int &socket2){
	for(unsigned int z = 0; z < partidas.size(); z++){
		if((partidas[z].getSocket1() == i) || (partidas[z].getSocket2() == i)){
			idPartida = z;
		}
	}

 socket1 = partidas[idPartida].getSocket1();
 socket2 = partidas[idPartida].getSocket2();
}

void salirAmbos(Partida &p, int socket1, int socket2, int &nPartidas){
	p.getJugador(socket1).salirPartida(&p);
	p.getJugador(socket2).salirPartida(&p);
	p.setSocket1(-1);
	p.setSocket2(-1);
	nPartidas--;
}


/*void salirCliente(int socket, fd_set * readfds, int * numClientes, int arrayClientes[]){

    char buffer[250];
    int j;

    close(socket);
    FD_CLR(socket,readfds);

    //Re-estructurar el array de clientes
    for (j = 0; j < (*numClientes) - 1; j++)
        if (arrayClientes[j] == socket)
            break;
    for (; j < (*numClientes) - 1; j++)
        (arrayClientes[j] = arrayClientes[j+1]);

    (*numClientes)--;

    bzero(buffer,sizeof(buffer));
    sprintf(buffer,"Desconexión del cliente: %d\n",socket);

    for(j=0; j<(*numClientes); j++)
        if(arrayClientes[j] != socket)
            send(arrayClientes[j],buffer,strlen(buffer),0);
}*/


void salirCliente(int socket, std::vector<Partida> &partidas, int &nPartidas, fd_set * readfds, int *numClientes, int arrayClientes[], fd_set * usuario_correcto, fd_set * usuario_validado, fd_set * usuario_esperandoPartida, fd_set * usuario_jugando, std::map<int, std::string> & usuarios){
	char buffer[250];
	int j;

	close(socket);
	FD_CLR(socket,readfds);


	if(FD_ISSET(socket, usuario_correcto)){
		FD_CLR(socket, usuario_correcto);
	}

	if(FD_ISSET(socket, usuario_validado)){
	   FD_CLR(socket, usuario_validado);
	}

	if(FD_ISSET(socket, usuario_esperandoPartida)){
	   FD_CLR(socket, usuario_esperandoPartida);
	}

	if(usuarios.find(socket)!=usuarios.end()){
	   usuarios.erase(socket);
	}

	//Re-estructurar el array de clientes
	for(j=0;j<(*numClientes)-1; j++){
	   if(arrayClientes[j]==socket){
	      break;
	   }
	}

	for(;j<(*numClientes)-1;j++){
	   (arrayClientes[j] = arrayClientes[j+1]);
	}

	(*numClientes)--;

	bzero(buffer,sizeof(buffer));
	sprintf(buffer,"Desconexión del cliente: %d\n",socket);

	for(j=0; j<(*numClientes); j++)
		 if(arrayClientes[j] != socket)
			  send(arrayClientes[j],buffer,strlen(buffer),0);

	//Solo si estaba jugando, cambiamos lo siguiente
	if (FD_ISSET(socket, usuario_jugando)){
		int idPartida, socket1, socket2;
		setIDPartidaySockets(socket, partidas, idPartida, socket1, socket2);

							salirAmbos(partidas[idPartida], socket1, socket2, nPartidas);

	 if(socket == socket1){
		 FD_SET(socket2, usuario_validado);
		 FD_CLR(socket2, usuario_jugando);

		 enviarMensaje(socket2,"+OK. Si quiere, puede iniciar otra partida.\n");
	 }
	 else{
		 FD_SET(socket1, usuario_validado);
		 FD_CLR(socket1, usuario_jugando);

		 enviarMensaje(socket1,"+OK. Si quiere, puede iniciar otra partida.\n");
	 }

	 FD_CLR(socket, usuario_jugando);
	}
}


void decidirTurnoInicial(Jugador &j1, Jugador &j2, Ficha &a, int socket1, int socket2, Partida &p){
	if(j1.existeFicha(a)){
		p.setTurno(socket1);
		enviarMensaje(socket1, "\n+OK. Turno de partida\n");
		enviarMensaje(socket2, "\n+OK. Turno del otro jugador\n");
	}
	else if(j2.existeFicha(a)){
		p.setTurno(socket2);
		enviarMensaje(socket2, "\n+OK. Turno de partida\n");
		enviarMensaje(socket1, "\n+OK. Turno del otro jugador\n");
	}
}


void decidirTurno(int i, int socket1, int socket2, Partida &p){
	if(i == socket1){
		p.setTurno(socket2);
		enviarMensaje(socket1, "\n+OK. Turno del otro jugador.\n");
		enviarMensaje(socket2, "\n+OK. Turno de partida.\n");
	}
	else{
		p.setTurno(socket1);
		enviarMensaje(socket2, "\n+OK. Turno del otro jugador.\n");
		enviarMensaje(socket1, "\n+OK. Turno de partida.\n");
	}
}


void nuevaPartida(int &nPartidas, vector<Partida> &partidas, int i, fd_set * usuario_esperandoPartida){
		Partida nuevo;
		nuevo.setSocket1(i);
		nuevo.setIDPartida(partidas.size());
		partidas.push_back(nuevo);
		nPartidas++;

		FD_SET(i, usuario_esperandoPartida);

		enviarMensaje(i,"+Ok. Petición Recibida. Quedamos a la espera de más jugadores\0");
}

void nuevaPartidaEnPosicion(int &nPartidas, vector<Partida> &partidas, int i, int pos, fd_set * usuario_esperandoPartida){
		Partida nuevo;
		nuevo.setSocket1(i);
		partidas[pos] = nuevo;
		partidas[pos].setIDPartida(pos);
		nPartidas++;

		FD_SET(i, usuario_esperandoPartida);

		enviarMensaje(i,"+Ok. Petición Recibida. Quedamos a la espera de más jugadores\0");
}



void mostrarFichaRobada(Ficha &a, int socket){
	char buffer[250];
	bzero(buffer,sizeof(buffer));
	sprintf(buffer, "%s\n", a.mostrarFicha().c_str());
	send(socket,buffer,strlen(buffer),0);
}

void mostrarManoJugador(Jugador &j, int socket){
	char buffer[250];
	bzero(buffer,sizeof(buffer));
	sprintf(buffer, "%s\n", j.mostrarMano().c_str());
	send(socket,buffer,strlen(buffer),0);
}


void mostrarTableroAJugador(int socket, Partida &p){
	char buffer[250];
	bzero(buffer,sizeof(buffer));
	sprintf(buffer, "%s\n", p.mostrarTablero().c_str());
	send(socket,buffer,strlen(buffer),0);
}

int otroSocket(int socket, Partida &p){
	if(socket == p.getSocket1()){
		return p.getSocket2();
	}
	else if(socket == p.getSocket2()){
		return p.getSocket1();
	}

	return 100000; //Solo para evitar warning
}



void lanzarPartida(Partida &p, Ficha &a, int i, fd_set * usuario_esperandoPartida, fd_set * usuario_jugando){
	int socket1=p.getSocket1();
	//char buffer[250];

	p.setSocket2(i);
	int socket2=p.getSocket2();

	enviarMensaje(i,"+OK. Empieza la partida.\n");
	enviarMensaje(socket1,"+OK. Empieza la partida.\n");

	Jugador j1(socket1, &p);
	p.nuevoJugador(&j1);

	Jugador j2(socket2, &p);
	p.nuevoJugador(&j2);

	mostrarManoJugador(j1, socket1);
	mostrarManoJugador(j2, socket2);


	a = p.iniciarPartida();
	p.setMasAlta(a);

	FD_CLR(socket1, usuario_esperandoPartida);
	FD_SET(socket1, usuario_jugando);
	FD_SET(i, usuario_jugando);

/*	bzero(buffer, sizeof(buffer));
	sprintf(buffer, "Partida %d:", p.getIDPartida());
	cout << buffer << endl;

	bzero(buffer, sizeof(buffer));
	sprintf(buffer, "Ficha mayor: %s\n", a.mostrarFicha().c_str());
	cout << buffer << endl << endl;*/

	//Comprobar quién tiene el doble más alto o la ficha más alta
	//Y decirle a este que es su turno y al otro que espere
	decidirTurnoInicial(j1, j2, a, socket1, socket2, p);
}


bool hayHueco(int nPartidas, vector<Partida> &partidas){
	for (size_t z = 0; z < partidas.size(); z++) {
		if(partidas[z].getSocket2() == -1){
			cout << "Hay hueco\n";
			return true;
		}
	}
	cout << "No hay hueco\n";
	return false;
};


void salirAmbosFinal(Partida &p, int socket1, int socket2, int &nPartidas, fd_set * usuario_validado, fd_set * usuario_jugando){
	p.getJugador(socket1).salirPartida(&p);
	p.getJugador(socket2).salirPartida(&p);
	p.setSocket1(-1);
	p.setSocket2(-1);
	nPartidas--;

	FD_SET(socket1, usuario_validado);
	FD_SET(socket2, usuario_validado);

	FD_CLR(socket1, usuario_jugando);
	FD_CLR(socket2, usuario_jugando);
}


void saberGanadorNoFichas(int i, int socket1, int socket2){
	enviarMensaje(i, "\n+OK. Ha ganado la partida.\n\n+OK. Si quiere, puede iniciar otra partida.\n");

	if(i == socket1){
		enviarMensaje(socket2, "\n+OK. Ha perdido la partida.\n\n+OK. Si quiere, puede iniciar otra partida.\n");
	}
	else{
		enviarMensaje(socket1, "\n+OK. Ha perdido la partida.\n\n+OK. Si quiere, puede iniciar otra partida.\n");
	}
}


void saberGanadorPuntos(Partida &p, int socket1, int socket2){
	if(p.getJugador(socket1).getPuntos() < p.getJugador(socket2).getPuntos()){
		enviarMensaje(socket1, "\n+OK. Ha ganado la partida.\n\n+OK. Si quiere, puede iniciar otra partida.\n");
		enviarMensaje(socket2, "\n+OK. Ha perdido la partida.\n\n+OK. Si quiere, puede iniciar otra partida.\n");
	}
	else if(p.getJugador(socket2).getPuntos() < p.getJugador(socket1).getPuntos()){
		enviarMensaje(socket2, "\n+OK. Ha ganado la partida.\n\n+OK. Si quiere, puede iniciar otra partida.\n");
		enviarMensaje(socket1, "\n+OK. Ha perdido la partida.\n\n+OK. Si quiere, puede iniciar otra partida.\n");
	}
	else if(p.getJugador(socket1).getPuntos() == p.getJugador(socket2).getPuntos()){
		enviarMensaje(socket1, "\n+OK. Se acabó la partida. Han empatado a puntos.\n\n+OK. Si quiere, puede iniciar otra partida.\n");
		enviarMensaje(socket2, "\n+OK. Se acabó la partida. Han empatado a puntos.\n\n+OK. Si quiere, puede iniciar otra partida.\n");
	}
}



#endif
