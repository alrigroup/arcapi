#ifndef RATELIMIT_H
#define RATELIMIT_H

/**
 * Inicializa a estrutura de memória do limitador de requisições.
 */
void ratelimit_init();

/**
 * Verifica se o IP excedeu a cota estabelecida.
 * Regras: 5 req/min para rotas de Login, 100 req/min para demais rotas.
 * 
 * @param ip IP do cliente
 * @param path Caminho solicitado
 * @return 1 se permitido, 0 se bloqueado (429)
 */
int ratelimit_check(const char *ip, const char *path);

#endif // RATELIMIT_H