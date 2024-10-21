#!/bin/bash

SERVER_IP="192.168.0.180"

PORT=53348

# Compilar cliente e servidor
gcc -Wall cliente.c -o cliente
gcc -Wall servidor.c -o servidor

echo "backlog,conexoes_bem_sucedidas" > resultados.csv

for backlog in {0..12}; do
    echo "Iniciando servidor com backlog = $backlog"
    ./servidor $backlog &  # Inicia o servidor em segundo plano
    SERVER_PID=$!  # Armazena o PID do servidor

    sleep 2  # Espera o servidor iniciar

    # CLIENT_COUNT=$((5 + RANDOM % 11))
    CLIENT_COUNT=10

    echo "Iniciando $CLIENT_COUNT clientes..."

    for ((i = 1; i <= CLIENT_COUNT; i++)); do
        ./cliente $SERVER_IP &
    done

    # Monitorar conexões com netstat no macOS
    # Espera as conexões estabilizarem
    # netstat -an lista todas as conexões e portas.
    sleep 1
    netstat -an | grep $PORT

    # Conta quantas conexões bem-sucedidas (aceitadas pelo servidor)
    CONEXOES_TOTAL=$(netstat -an | grep "$PORT.*ESTABLISHED" | wc -l)
    # Divide o valor por 2, pois há uma conexão listada tanto para o lado cliente quanto servidor
    CONEXOES_BEM_SUCEDIDAS=$((CONEXOES_TOTAL / 2))
    echo "$backlog,$CONEXOES_BEM_SUCEDIDAS" >> resultados.csv

    # Finalizar servidor e garantir que a porta seja liberada
    kill -9 $SERVER_PID
    wait $SERVER_PID 2>/dev/null
    sleep 2  # Pausa para liberar a porta

    echo "Servidor com backlog $backlog finalizado."
    echo "------------------------------"
done

echo "Testes concluídos!"
