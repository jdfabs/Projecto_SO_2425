# Projeto Sudoku - SO 2425 - Cliente/Servidor
# João Silva 2064413
# Samuel Andrade 2068220
# Rosa ----

## Descrição
Este projeto consiste na implementação de uma aplicação cliente/servidor para o jogo Sudoku, com o objetivo de praticar conceitos de concorrência, sincronização e comunicação, utilizando a linguagem C.

Nesta primeira fase, foram implementadas as funcionalidades de leitura de ficheiros de configuração, registo de eventos (logging) e verificação de soluções de Sudoku.

## Estrutura do Projeto

O projeto está dividido nos seguintes ficheiros e diretórios principais:

├── src/
    ├── client/
        ├── client.c                # Implementação do Cliente
        ├── client_config.c         # Carregamento das Configs a partir de JSON
    ├── server/
        ├── server.c                # Implementação do Servidor
        ├── server_config.c         # Carregamento das Configs a partir de JSON
        ├── server_stats.c          # TODO - estatisticas
        ├── game_manager.c          # TODO - manager de "rooms"
        ├── sudoku_solver.c         # Verificação de soluções
    ├── common/
        ├── coms.c                  # TODO - Comunicação entre processos
        ├── file_handler.c          # Leitura/escrita de ficheiros
├── include/                   # Diretório com alguns ficheiros .h
├── logs/                      # Diretório onde são guardados os logs
├── config/                    # Diretório onde estão os ficheiros de configuração
├── Makefile                   # Script de compilação

## Funcionalidades Implementadas

### 1. Leitura de Configurações
O cliente e o servidor carregam os parâmetros de configuração a partir de ficheiros `.conf` localizados no diretório `config/`. Estes ficheiros contêm informações como o IP do servidor, portas de comunicação, IDs do cliente, entre outros. 

Por enquanto apenas são lidos e impressos na consola.

### 2. Logging (Registo de Eventos)
O sistema Regista ações que ocorrem no cliente e no servidor. Estes logs são guardados no diretório `logs/` e incluem eventos como a leitura dos ficheiros de configuração e validação das soluções de Sudoku.

### 3. Verificação de Soluções de Sudoku
Uma função de verificação de tabuleiros de Sudoku verifica se uma solução é válida. 
Outra que conta quantas células estão incorretas.

Celulas a "0" implica que estas ainda não foram preenchidas.

## Como Compilar

Para compilar o projeto, execute o seguinte comando na raiz do projeto:

make all        ---  O comando compila o cliente e o servidor. 
make clean      ---  O comando faz limpeza de todos os ficheiros compilados.

## Como Executar
### Executar o Servidor
Para iniciar o servidor, execute o seguinte comando na raiz do projeto:

./server

### Executar o Cliente
Para iniciar um cliente, execute o seguinte comando na raiz do projeto:

./client [NOME_DO_FICHEIRO_DE_CONFIGURAÇÃO]

Onde [NOME_DO_FICHEIRO_DE_CONFIGURAÇÃO] é um parametro opcional de modos que se não for passado o ficheiro do cliente 1 será utilizado.

## Exemplo de Configuração (server.json e client_#.json)

### server.json:
    {
        "server": {
            "ip": "127.0.0.1",
            "port": 8080,
            "enable_logging": 1,
            "log_file": "./logs/server_log.txt",
            "log_level": 1,
            "max_clients": 5,
            "backup_interval": 15
        }
    }

### client_#.json:
    {
        "client": {
            "id": "client1",
            "server_ip": "127.0.0.1",
            "server_port": 8080,
            "log_file": "./logs/client1_log.txt"
        }
    }

