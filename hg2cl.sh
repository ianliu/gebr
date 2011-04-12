hg --quiet log --style=changelog $* | perl -pe '
s/  braulio.*>$/  Braulio Barros de Oliveira  <brauliobo\@gebrproject.com>/i;
s/  (ian|ianliu).*>$/  Ian Liu Rodrigues  <ian.liu\@gebrproject.com>/i;
s/  biloti.*>$/  Ricardo Biloti  <biloti\@gebrproject.com>/;
s/  fmatheus.*>$/  Fabrício Matheus Gonçalvez  <fmatheus\@gebrproject.com>/;
s/  (alexandre|abaaklini).*>$/  Alexandre Baaklini Gomes Coelho  <abaaklini\@gebrproject.com>/i;
s/  rodrigo.*>$/  Rodrigo Morelatto  <morelatto\@gebrproject.com>/i;
s/  jorge.*>$/  Jorge Pizzolatto <jorge.pzt\@gebrproject.com>/i;
s/  sobral.*>/  Gabriel Sobral <sobral\@gebrproject.com>/i;
s/  davi.*>$/  Davi Clemente <davi.clemente\@gebrproject.com>/i;
s/  (fabioaz|fabiosazevedo).*>$/  Fabio de Souza Azevedo  <fabioaz\@gebrproject.com>/' | tail -n +2
echo
