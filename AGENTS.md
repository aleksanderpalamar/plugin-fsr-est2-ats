# AGENT.md

Você deve respeitar as regras deste projeto em todas as alterações realizadas.
Todo código produzido deve permanecer em conformidade com estas regras.

## Interação com Claude

Você é o agente principal responsável pela execução das tarefas deste projeto.

Claude deve ser utilizado como um par técnico para discussão, revisão e investigação quando sua participação puder contribuir para a resolução da tarefa.

Não consulte Claude automaticamente para toda tarefa.

Consulte Claude quando ocorrer pelo menos uma das seguintes situações:

- o problema não possuir uma solução clara após a análise inicial;
- houver múltiplas abordagens arquiteturais relevantes;
- a tarefa envolver uma mudança complexa ou de grande impacto;
- você estiver investigando um bug cuja causa não esteja clara;
- uma primeira tentativa de implementação ou correção não resolver o problema;
- houver dúvida sobre uma decisão técnica importante;
- for útil obter uma segunda análise antes de finalizar uma solução complexa.

Para tarefas simples, mecânicas ou cuja solução esteja clara, resolva diretamente sem iniciar uma sessão com Claude.

### Sessão

Quando for necessário consultar Claude, execute:

`claude`

Mantenha a conversa focada na tarefa atual e forneça o contexto técnico necessário para que Claude possa colaborar na análise.

Ao sair da sessão utilizando `/quit`, Claude apresentará um comando semelhante a:

`claude --resume <session-id>`

Salve o comando de retomada da sessão no arquivo `session.md`.

Se uma sessão relacionada à tarefa atual já estiver registrada em `session.md`, reutilize essa sessão em vez de iniciar uma nova.

### Ciclo de colaboração

Durante uma tarefa complexa, você pode alternar entre:

1. analisar o problema;
2. discutir hipóteses ou alternativas com Claude;
3. investigar e implementar a solução;
4. executar testes e validações;
5. retornar à mesma sessão do Claude quando surgir nova informação relevante ou quando uma segunda análise puder ajudar.

Não é necessário retornar ao Claude após cada alteração ou descoberta.

A implementação, validação e decisão final permanecem sob sua responsabilidade.

Sugestões fornecidas por Claude não devem ser consideradas corretas automaticamente. Verifique-as utilizando o código existente, documentação, compilação, testes e demais mecanismos de validação do projeto.

## Princípios de arquitetura

- Aplicar o Princípio da Responsabilidade Única (SRP).
  Cada módulo, struct, função ou componente deve possuir uma responsabilidade clara e bem definida.

- Aplicar o Princípio da Inversão de Dependência (DIP).
  Módulos de alto nível não devem depender diretamente de módulos de baixo nível.
  Ambos devem depender de abstrações.

## Organização do código

- Seguir princípios de Clean Code.

- Cada arquivo de código-fonte deve possuir, preferencialmente, no máximo 200 linhas.
  Caso ultrapasse esse limite, avaliar a divisão do arquivo em módulos menores e coesos.

- Para cada alteração, ajustes, correções de bugs e etc, deve ser realizados em branches separadas.

-  Não adicionar comentarios no código sem a minha permissão.

- Funções devem ser pequenas e possuir uma única responsabilidade.

- Evitar condicionais profundamente aninhadas.

- Preferir:
  - early return;
  - `match`;
  - `if let`;
  - `let else`;
  - decomposição em funções menores;
  - tipos e enums para representar estados explicitamente.

- Evitar duplicação de código.
  Quando houver comportamento realmente compartilhado, extrair uma abstração apropriada.

- Não criar abstrações prematuramente.
  Uma abstração deve existir porque resolve um problema real de design,
  e não apenas para antecipar uma possível necessidade futura.

## Códigos de conduta

Você pode ser proativo dentro do contexto da tarefa, mas nunca fora dele. Se, durante uma tarefa, você perceber que um trecho de código precisa de refatoração ou correção que não foi solicitada, não a execute. Apenas registre o que foi identificado no relatório final, para que eu decida se entra em uma próxima tarefa.

## C++

- Priorizar código idiomático em C++.

- Utilizar o sistema de tipos para representar regras e estados sempre que possível.

- Preferir enums em vez de flags booleanas quando existirem múltiplos estados possíveis.
Evitar operações que assumem sucesso sem verificar erros.
Em Option<T>, evitar unwrap() e expect() quando a ausência for um caso normal sem antes tratar isso.
Evitar panic!, unreachable! e assert! para situações esperadas de execução.
Preferir tratamento explícito do erro.

- Utilizar Option<T> quando um valor pode não existir.

- Não silenciar warnings sem uma justificativa clara.

## Testes

- Todo comportamento relevante deve possuir testes.

- Algoritmos e regras de domínio devem ser testáveis.

- Correções de bugs devem, sempre que possível, incluir um teste que reproduza o problema antes da correção.

- Funções determinísticas de domínio devem possuir testes unitários.

## Qualidade

Antes de considerar uma alteração concluída:

1. O projeto deve compilar sem erros.
2. Novos warnings não devem ser introduzidos sem justificativa.
3. Os testes existentes devem continuar passando.
4. Novos comportamentos devem possuir testes quando aplicável.
5. O código deve permanecer simples, legível e consistente com a arquitetura existente.