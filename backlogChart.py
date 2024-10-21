import matplotlib.pyplot as plt
import pandas as pd

# Carregar os dados do CSV gerado pelo script de teste
dados = pd.read_csv('resultados.csv')

# Configurar o gráfico
plt.figure(figsize=(10, 6))
plt.plot(dados['backlog'], dados['conexoes_bem_sucedidas'], marker='o', linestyle='-', color='b')
plt.xlabel('Backlog')
plt.ylabel('Número de Conexões Bem-Sucedidas')
plt.title('Impacto do Backlog nas Conexões Bem-Sucedidas')
plt.grid(True)
plt.xticks(dados['backlog'])

# Mostrar o gráfico
plt.savefig('impacto_backlog_conexoes.png')  # Salvar como imagem
plt.show()  # Mostrar na tela
