# Exercício 7: Serviço de Bate-Papo com Notificações de Status 

### Serviço de Bate-Papo com Notificações de Status


**Descrição Geral**

Este projeto implementa um serviço de bate-papo em linguagem C que permite a comunicação entre múltiplos clientes conectados a um servidor centralizado. O sistema utiliza:

- **TCP** para troca de mensagens de bate-papo, garantindo a entrega confiável das mensagens.
- **UDP** para envio de notificações rápidas de status, como entradas e saídas de usuários.

O servidor é capaz de gerenciar múltiplos clientes simultaneamente, encaminhando mensagens e notificações conforme necessário.

---

**Pré-requisitos**

- Sistema operacional **Linux/Unix**.
- Compilador **GCC** com suporte a **POSIX threads**.
- Bibliotecas padrão do C (nenhuma biblioteca externa é necessária).

---

**Compilação**

Os códigos fonte estão divididos em dois arquivos:

- `servidor.c`: Código do servidor.
- `cliente.c`: Código do cliente.

Para compilar os arquivos, siga as instruções abaixo:

**Compilando o Servidor**

Abra um terminal na pasta onde o arquivo `servidor.c` está localizado e execute o comando:

```
gcc -Wall -o servidor servidor.c -pthread
```

- `-Wall`: Ativa todos os warnings durante a compilação.
- `-o servidor`: Especifica o nome do executável gerado.
- `-pthread`: Linka a biblioteca de threads POSIX necessária.

**Compilando o Cliente**

No mesmo diretório, compile o cliente com o comando:

```
gcc -Wall -o cliente cliente.c -pthread
```

---

**Execução**

**Servidor**

Para iniciar o servidor, execute o comando abaixo, substituindo `<PORTA_TCP>` pela porta que deseja utilizar (por exemplo, `5002`):

```
./servidor <PORTA_TCP>
```

**Exemplo:**

```
./servidor 5002
```

**Notas:**

- Certifique-se de que a porta escolhida não esteja sendo utilizada por outro serviço.
- O servidor deve ser iniciado **antes** dos clientes se conectarem.

**Cliente**

Para iniciar um cliente, execute o comando abaixo em um terminal separado, substituindo `<IP_DO_SERVIDOR>` pelo endereço IP do servidor (use `127.0.0.1` se estiver executando localmente) e `<PORTA_TCP>` pela mesma porta utilizada ao iniciar o servidor:

```
./cliente <IP_DO_SERVIDOR> <PORTA_TCP>
```

**Exemplo:**

```
./cliente 127.0.0.1 5002
```

**Notas:**

- Você pode iniciar múltiplos clientes em terminais diferentes para simular vários usuários.
- Certifique-se de que o endereço IP e a porta correspondem ao servidor em execução.

---

**Como Utilizar**

1. **Inicie o servidor** conforme instruções anteriores.

2. **Inicie o cliente**:

   - Ao iniciar, o cliente solicitará que você insira seu **nickname**.
   - Digite um nome de usuário que será exibido nas mensagens e notificações.

3. **Envio de Mensagens**:

   - Após conectar-se, você pode digitar mensagens que serão enviadas para todos os outros usuários conectados.
   - As mensagens recebidas dos outros usuários serão exibidas no terminal.

4. **Notificações de Status**:

   - Quando um usuário entra ou sai do chat, uma notificação será exibida no formato `[NOTIFICAÇÃO]: Usuário entrou/saiu do chat.`

**Comandos Disponíveis**

- **`/sair`**: Digite `/sair` para desconectar-se do chat e encerrar o cliente.

---

**Funcionamento Interno**

**Comunicação TCP**

- **Mensagens de Bate-Papo**:
  - Utiliza o protocolo TCP para garantir a entrega confiável das mensagens.
  - O servidor recebe a mensagem de um cliente e a encaminha para todos os outros clientes conectados.

**Comunicação UDP**

- **Notificações de Status**:
  - Utiliza o protocolo UDP para enviar notificações rápidas de entrada e saída de usuários.
  - Cada cliente faz o **bind** em uma porta UDP dinâmica (escolhida pelo sistema operacional), evitando conflitos.
  - O servidor envia notificações para a porta UDP específica de cada cliente.

---

**Log de Eventos**

- O servidor registra eventos importantes em um arquivo chamado `servidor.log`.
- Informações registradas:
  - Conexões e desconexões de clientes.
  - Mensagens trocadas entre os usuários.
- O arquivo `servidor.log` é criado no diretório onde o servidor está sendo executado.

---

**Considerações Importantes**

- **Sincronização**:
  - O servidor utiliza mutexes para sincronizar o acesso à lista de clientes entre múltiplas threads.

- **Tratamento de Erros**:
  - O código verifica os retornos das funções críticas e imprime mensagens de erro para facilitar a depuração.

- **Desconexão de Clientes**:
  - O servidor lida corretamente com clientes que se desconectam inesperadamente.

- **Limitações**:
  - O número máximo de clientes simultâneos é definido por `MAX_CLIENTS` no código (padrão: 100).

---

