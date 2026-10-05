---
name: tdd
description: Aplique desenvolvimento guiado por testes ao criar funcionalidades ou corrigir bugs neste plugin. Use esta skill sempre que uma tarefa alterar comportamento de C++, shaders, configuração, LUTs ou empacotamento. Defina cenários observáveis, escreva e execute primeiro um teste que falha, implemente a menor correção, refatore e valide os checks relevantes do projeto.
---

# TDD neste projeto

Use o ciclo vermelho, verde e refatoração para mudanças de comportamento. Leia `AGENTS.md` e os testes existentes antes de editar. Siga o escopo pedido pelo usuário e trabalhe na branch apropriada.

## Escolha os cenários

1. Descreva o comportamento esperado em termos observáveis. Para bugs, reproduza a entrada e o resultado incorreto. Para features, derive casos de aceitação dos requisitos.
2. Selecione casos relevantes: fluxo normal, limites, entradas inválidas, estados ausentes e regressões plausíveis. Priorize impacto e probabilidade; não tente enumerar todos os bugs possíveis.
3. Use o nível de teste mais próximo da regra: unitário para funções determinísticas, integração para a interação entre componentes e testes de pacote para o ZIP distribuído. Prefira interfaces públicas e dados reais pequenos a testes que apenas copiam a implementação.
4. Planeje uma pequena mudança por ciclo. Se o requisito ainda estiver ambíguo, esclareça o comportamento antes de fixá-lo em testes.

## Vermelho

1. Escreva primeiro um teste automatizado que expresse o próximo comportamento. Para um bug, faça o teste reproduzir a falha antes da correção.
2. Execute o menor alvo de teste disponível que contenha o novo caso e confirme que falha pelo motivo esperado. Uma falha de ambiente, dependência ou sintaxe não comprova o comportamento.
3. Preserve a evidência da falha no relato. Não altere o código de produção antes de observar o vermelho, salvo a menor estrutura necessária para compilar o teste.

## Verde e refatoração

1. Implemente apenas o suficiente para passar o teste. Evite corrigir outros comportamentos ou refatorar áreas fora do pedido.
2. Execute novamente o teste até ficar verde. Em seguida, rode os testes próximos para detectar regressões.
3. Refatore somente o trecho alterado quando isso melhorar a clareza. Mantenha os testes verdes; aplique SRP, DIP, funções pequenas, retornos antecipados e as demais regras de `AGENTS.md`.
4. Repita o ciclo para cada cenário relevante. Não acrescente testes redundantes só para aumentar a contagem.

## Onde validar

- Regras de configuração e leitura de LUT: use os testes C++ executados por CMake e CTest. Mantenha os casos determinísticos e independentes de GPU.
- DXGI, D3D11 e shaders: use os testes de integração e compilação no Windows quando o comportamento depender dessas APIs. Extraia lógica pura apenas quando houver uma separação real de responsabilidades.
- Scripts e conteúdo do ZIP: use os testes Python de `tests/package_check_test.py` e a verificação de `tests/package_check.py` quando a mudança afetar a distribuição.
- Consulte `.github/workflows/pr-quality-gate.yml` para os comandos atuais de Linux e Windows. Execute localmente o que o ambiente permitir e acompanhe o CI quando houver PR.

Se o comportamento visual ou dependente do jogo não tiver um teste automatizado confiável, teste primeiro as regras determinísticas e os contratos verificáveis disponíveis. Documente a lacuna e valide no jogo quando possível. Não apresente um teste de compilação de shader como prova de qualidade visual nem afirme que houve TDD completo sem observar o vermelho.

## Relato

Informe quais cenários foram cobertos, qual teste falhou antes da implementação, quais comandos passaram depois e quais validações ficaram pendentes. Descreva limitações de plataforma ou de teste visual com precisão.
