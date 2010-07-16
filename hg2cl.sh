hg --quiet log -M --style=changelog $* | perl -pe '
s/  brauliobo$/ Braulio Barros de Oliveira <brauliobo\@gebrproject.com>/;
s/  (ian|ianliu)$/  Ian Liu Rodrigues <ian.liu\@gebrproject.com>/;
s/  biloti$/  Ricardo Biloti <biloti\@gebrproject.com>/;
s/  fmatheus$/  Fabrício Matheus Gonçalvez <fmatheus\@gebrproject.com>/;
s/  abaaklini$/  Alexandre Baaklini Gomes Coelho <abaaklini\@gebrproject.com>/;
s/  (fabioaz|fabiosazevedo)$/  Fabio de Souza Azevedo <fabioaz\@gebrproject.com>/'
