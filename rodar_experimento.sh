#!/bin/bash

make clean
make

MATRICULA="002739"
A=$((100000000 + 10#$MATRICULA))
B=8000000000

SAIDA="resultados.csv"

rm -f $SAIDA

echo "Iniciando os testes..."

for W in 1 2 4 8; do
    for modo in thread processo; do
        for particao in bloco ciclico; do
            echo "Rodando: W=$W Modo=$modo Particao=$particao"
            ./varredor $A $B $W $modo $particao $SAIDA
        done
    done
done

echo "Finalizado! Resultados salvos em $SAIDA"
