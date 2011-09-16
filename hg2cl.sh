# Usage example:
#
# ./hg2cl.sh -r "sort(0.13.0::.-branch(0.12),-date)+sort(3888::0.12.0,-date)" . > x && mv x ChangeLog
#
# PS: 3888 was the last revision where ChangeLog was updated

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
s/  pris.*>$/  Priscila Moraes de Souza <pris.moraess\@gebrproject.com>/i;
s/  giuliano.*>$/  Giuliano Roberto Pinheiro <giuliano\@gebrproject.com>/i;
s/  (keiji|eric).*>$/  Eric Keiji <keiji.eric\@gebrproject.com>/i;
s/  (fabioaz|fabiosazevedo).*>$/  Fabio de Souza Azevedo  <fabioaz\@gebrproject.com>/' \
| perl -ne '(/^2/ && print "\n$_\n") || /^$/ || print' | tail -n +2
echo | paste -s -d\\n - ChangeLog
