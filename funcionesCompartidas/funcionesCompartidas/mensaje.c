#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>

char *armar_mensaje(char *identificador, char *mensaje)
{
	char *resultado = strdup(identificador);

	char *payload_size   = string_itoa(string_length(mensaje));
	int   payload_digits = string_length(payload_size);

	string_append(&resultado, string_repeat('0', 10 - payload_digits));
	string_append(&resultado, payload_size);
	string_append(&resultado, mensaje);

	free(payload_size);
	return resultado;
}

char *get_header(char *mensaje)
{
	return string_substring(mensaje, 0, 1);
}

int comparar_header(char *identificador, char *header)
{
	return !strcmp(header, identificador);
}

int get_codigo(char *mensaje)
{
	int codigo;
	char *cod = string_substring(mensaje, 1, 2);
	codigo = atoi(cod);
	free(cod);
	return codigo;
}

char *get_mensaje(char *mensaje)
{
	char *payload = string_substring(mensaje, 3, 10);
	int payload1 = atoi(payload);
	free(payload);
	return string_substring(mensaje, 13, payload1);
}
