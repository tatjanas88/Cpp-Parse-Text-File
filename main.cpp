#include<stdlib.h>
#include<stdio.h>
#include<string>
#include<regex>
#include<iostream>
#include <fcntl.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/mman.h>
/*boje u 16-bitnoj interpretaciji*/
#define BLUE 0x001F
#define GREEN 0x07E0
#define RED 0xF800
#define BLACK 0x0000
#define YELLOW (RED + GREEN)
#define VGA_X 640
#define VGA_Y 480

#define MAX_MMAP_SIZE (VGA_X * VGA_Y * sizeof(unsigned int)) //zato sto nemaju svi procesori istu sirinu tipova(int moze biti 16,32,64)
//svaki piksel je jedan INT
void lineh_draw(int x1, int x2, int y, int color, int *buf)//funkcija za crtanje vertikalne l. (proslijedjuju se koord.,boja i pok.na memoriju koju ce prebojiti)
{
    for(int x=x1; x<=x2; x++)
    {   /*zato sto memorija ima jednu dimenziju---redove moramo da dodamo na kolone-kao offset*/
        buf[y*VGA_X + x] = color;
    }
}
void linev_draw(int y1, int y2, int x, int color, int *buf)//crtanje horiz.linije
{
    for(int y=y1; y<=y2; y++)
    {   /*isto kao u prethodnom slucaju*/
        buf[y*VGA_X + x] = color;
    }
}
void rect_draw(int x1, int x2, int y1, int y2, int color, int *buf)//crtanje pravougaonika
{
    for (int x=x1;x<=x2;x++)
    {
        for(int y=y1; y<=y2; y++)
        {
            buf[y*VGA_X + x] = color;//ovo vazi u svakom slucaju
        }
    }
}
void background_fill(int color, int *buf)//bojenje pozadine je isto kao bojenje pravougaonika velicine(rezolucije) ekrana
{
    rect_draw(0, VGA_X-1,0, VGA_Y-1, color, buf);
}
int get_color(std::string c) //funkcija koja ce pokupiti string=boju
{
  if (c == "BLUE")
    return BLUE;
  if (c == "GREEN")
    return GREEN;
  if (c == "RED")
    return RED;
  if (c == "BLACK")
    return BLACK;
  if (c == "YELLOW")
    return YELLOW;

  return BLACK;
}
void regex_line(std::string line, int *buffer)//funkcija regex prima liniju po liniju mog text fajla i pokazivac tipa int na fajl
{
    std::regex command_regex("(BCKG|LINE_V|LINE_H|RECT):"); //regex za trazenje komandi
    std::smatch match; //promjenjiva tipa smatch koja ce sadzati ono sto je regex nasao

    if (std::regex_search(line, match, command_regex))//funkcija regex search prima liniju fajla, ono sto je pronasao regex i komande iz command_regex
    {
        if (match[1] == "BCKG") //ako je pronasao BCKG  pokrecemo novi regex da trazi boju
        {
            std::regex color_regex("(BLUE|RED|GREEN|BLACK|YELLOW)");
            if(std::regex_search(line, match, color_regex))//ovdje regex pretrazuje koja je boja
            {
                std::cout<< "Boja je: "<< match[1]<<"\n"; //u match[0] se smjesta cijeli string koji je nadjen, a u match[1] ono iz grupa
                background_fill(get_color(match[1]), buffer);//posto smo nasli BCKG i boju onda pozovemo f. za bojenje pozadine
            }//get_color prima string i vrsi provjeru boje...zatim vraca u f. background_fill odg. boju
        }
        if (match[1]== "LINE_V")///regex sve brojeve cuva kao strinogve zato nam trreba stoi
        {
            std::regex vline_regex("(\\d+), (\\d+), (\\d+); (BLUE|RED|GREEN|BLACK|YELLOW)");//regex sadrzi grupe brojeva(+ govori da ih moze biti vise)
             if(std::regex_search(line, match, vline_regex))
            {
                std::cout<< "LINE_V je: "<< match[1]<< " "<< match[2]<<" "<< match[3]<<" "<< match[4]<<"\n"; //daje brojeve koji su nadjeni u regex grupama i jednu boju
                linev_draw(std::stoi(match[1]), std::stoi(match[2]), std::stoi(match[3]), get_color(match[4]), buffer);
            }
        }
        if (match[1]== "LINE_H")
        {
            std::regex hline_regex("(\\d+), (\\d+), (\\d+); (BLUE|RED|GREEN|BLACK|YELLOW)");
             if(std::regex_search(line, match, hline_regex))
            {
                std::cout<< "LINE_H je: "<< match[1]<< " "<< match[2]<<" "<< match[3]<<" "<< match[4]<<"\n";
                lineh_draw(std::stoi(match[1]), std::stoi(match[2]), std::stoi(match[3]), get_color(match[4]), buffer);
            }
        }
        if (match[1]== "RECT")
        {
            std::regex rect_regex("(\\d+), (\\d+), (\\d+), (\\d+); (BLUE|RED|GREEN|BLACK|YELLOW)");
             if(std::regex_search(line, match, rect_regex))
            {
                std::cout<< "RECT je: "<< match[1]<< " "<< match[2]<<" "<< match[3]<<" "<< match[4]<<" "<< match[5]<<"\n";
                rect_draw(std::stoi(match[1]), std::stoi(match[2]), std::stoi(match[3]), std::stoi(match[4]),get_color(match[5]), buffer);
            }
        }
    }
    else
    {
        std::cout<< "Matching not found!\n";
    }
}
int main(int argc, char *argv[])
{
    FILE* pok; //pokazivac na fajl,odnosno na putanju
    char *line_buff; //pokazivac na lokaciju gdje ce se smjestati sadrzaj fajla
    size_t line_buff_size = 0; //velicina promjenjine u koju ce se smjestati sadrzaj fajla = u pocetku 0;

    if (argc < 2) {
      std::cout << "Please enter config file path\n";
      return -1;
    } else {
      pok = fopen (argv[1], "r"); //otvaranje text fajla
    }

    if (pok == NULL)
    {
        printf("Failed to open\n");
        exit(1);
    }
    //ovaj dio je preuzet sa git-a//
    int *buffer;
    int fd;
    fd = open("/dev/vga_dma", O_RDWR | O_NDELAY);
    if (fd < 0)
    {
      std::cout << "Cannot open " << "/dev/vga_dma" << "\n";
      exit(EXIT_FAILURE);
    }
    else
        /* memorijski mapiraj vga_dma na buffer*/
    buffer = (int *)mmap(0, MAX_MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        /*&line_buff je pokazivac na buffer gdje ce funk.smjestiti podatke procitane iz fajla=nealociran i prazan*/
        /*pokazivac na promenjivu koja sadrzi velicinu bafera=u mom slucaju to je nula, a poslije citanja ce sadrzati broj procitanih bajtova iz fajla*/
        /*pok je pokazivac na moj fajl*/
    while(getline(&line_buff, &line_buff_size, pok)>=0)
        {
        regex_line(line_buff, buffer);
        }
    fclose(pok); //yatvaranje text fajla
    munmap(buffer, MAX_MMAP_SIZE);
    close(fd);
    if (fd < 0)
      std::cout << "Cannot close " << "/dev/vga_dma" << "\n";

    delete [] buffer;
}
