#!/bin/bash

# Garante que o script tenha permissão de execução em si mesmo (auto-correção)
chmod +x "$0" 2>/dev/null

# Entra na pasta source. Se a pasta não existir, o script para aqui (-e)
cd source || { echo "Erro: Pasta 'source' não encontrada!"; exit 1; }

echo "Compilando ALRI-CORE..."

gcc core.c -o core -pthread

echo "Movendo binários para a pasta principal..."

mv core ../core

chmod +x core 

echo "--------------------------------"
echo "Compilacao do ALRI-CORE    Concluida!"
echo "Execute: sudo ./core"
echo "--------------------------------"