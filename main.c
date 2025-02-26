//-------------------------------------------- Bibliotecas --------------------------------------------
#include <stdio.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "pixel.h"

//--------------------------------------------- Variáveis ---------------------------------------------
//------------------------------------------- Configuração --------------------------------------------
const uint botao_a = 5;
const uint botao_b = 6;
const uint pixel = 7;

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;
const uint8_t endereco = 0x3C;
#define I2C_PORT i2c1
ssd1306_t ssd;

const uint botao_joy = 22;
const uint eixo_x = 26;
const uint eixo_y = 27;

static volatile uint32_t ultimo_tempo = 0;

//--------------------------------------------- Variáveis ---------------------------------------------
//----------------------------------------------- Geral -----------------------------------------------
bool fim_de_jogo = true;
int ultima_direcao = 0, atual_direcao=0, contador_centena = 0, contador_dezena = 0, contador = 0;


//--------------------------------------------- Variáveis ---------------------------------------------
//----------------------------------------------- Menu ------------------------------------------------
bool Iniciar = false, Jogando = false, Relatorio=false;
int contagem_relatorio = 0;
struct repeating_timer timer;


//---------------------------------------------- Funções ----------------------------------------------
//----------------------------------------------- Menu ------------------------------------------------
bool repeating_timer_callback(struct repeating_timer *t) {

    if(gpio_get(botao_joy)==0){
        contagem_relatorio++;
        if(contagem_relatorio==100){
            Relatorio = !Relatorio;
            contagem_relatorio=0;
            return false;
        }
        return true;

    }else{
        contagem_relatorio=0;
        return false;
    }
}

static void gpio_irq_handler(uint gpio, uint32_t events){

    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());

    if (tempo_atual - ultimo_tempo > 200000){

        switch(gpio){
            case botao_a:
                if(Iniciar==false){
                    Iniciar = true;
                }else{
                    if(Jogando==false&&Relatorio==false){
                        Jogando = true;
                    }
                }

                break;

            case botao_joy:
                if(Iniciar){
                    add_repeating_timer_ms(50, repeating_timer_callback, NULL, &timer);
                }
                break;

        }
    }
    ultimo_tempo = tempo_atual;
}


//--------------------------------------------- Variáveis ---------------------------------------------
//----------------------------------------------- Snake -----------------------------------------------
typedef struct cobra{
    int parte[2];
    struct cobra *next;

}cobra;

int fruta[2] = {42, 30};
cobra *corpo = NULL;
cobra *ptr_checagem = NULL;


//---------------------------------------------- Funções ----------------------------------------------
//----------------------------------------------- Snake -----------------------------------------------
void repetidor(){
    srand(time(NULL));
    ptr_checagem = corpo;
    fruta[0] = ((rand()%15)*4)+2;
    fruta[1] = ((rand()%15)*4)+2;
    while(ptr_checagem!=NULL){
        if((ptr_checagem->parte[0])==fruta[0]&&(ptr_checagem->parte[1])==fruta[1]){
            repetidor();
        }
        ptr_checagem = ptr_checagem->next;
    }
}

static void gpio_irq_handler_snake(uint gpio, uint32_t events){

    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());

    if (tempo_atual - ultimo_tempo > 200000){
        if(gpio==botao_a){
            if(fim_de_jogo == true && Jogando == false){
                fim_de_jogo = false;
            }
        }
    }
    ultimo_tempo = tempo_atual;
}

//--------------------------------------------- Variáveis ---------------------------------------------
//---------------------------------------------- Memoria ----------------------------------------------
bool verificar = false, selecao_de_nivel = true;


//---------------------------------------------- Funções ----------------------------------------------
//---------------------------------------------- Memoria ----------------------------------------------
static void gpio_irq_handler_memoria(uint gpio, uint32_t events){

    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());

    if (tempo_atual - ultimo_tempo > 200000){
        if(gpio == botao_b){
            verificar = true;
        }else{
            if(gpio==botao_a){
                if(selecao_de_nivel){
                    selecao_de_nivel = false;
                }else{
                    if(fim_de_jogo == true && Jogando==false){
                        fim_de_jogo = false;
                    }
                }
            }
        }
    }
    ultimo_tempo = tempo_atual;
    
}


//----------------------------------------------- JOGO ------------------------------------------------
//----------------------------------------------- Snake -----------------------------------------------
uint32_t Snake(){

    uint32_t snake_tempo_inicial = to_ms_since_boot(get_absolute_time()), snake_tempo_final = 0;

    atual_direcao = 1;

    srand(time(NULL));

    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 0,0,64,64, true, false);
    ssd1306_rect(&ssd, 1,1,62,62, true, false);

    ssd1306_draw_string(&ssd, "Score", 77, 20);
    ssd1306_send_data(&ssd);

    for(int i=0; i<3;i++){
        cobra *a = malloc(sizeof(cobra));
        a->parte[0]=30-(i*4);
        a->parte[1]=30;

        a->next=corpo;

        corpo = a;
    }

    cobra *ptr = corpo;
    cobra *ptr2 = corpo->next;

    gpio_set_irq_enabled_with_callback(botao_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler_snake);

    while(Jogando){
        adc_select_input(0);
        uint16_t vry_value = adc_read();
        adc_select_input(1);
        uint16_t vrx_value = adc_read();
        
        if(((vry_value<=2450)&&(vry_value>=1650)&&(vrx_value>2450))&&(ultima_direcao!=2)){
            atual_direcao = 1;
        }else{
            if(((vry_value<=2450)&&(vry_value>=1650)&&(vrx_value<1650))&&(ultima_direcao!=1)){
                atual_direcao = 2;
            }else{
                if(((vrx_value<=2450)&&(vrx_value>=1650)&&(vry_value>2450))&&(ultima_direcao!=4)){
                    atual_direcao = 3;
                }else{
                    if(((vrx_value<=2450)&&(vrx_value>=1650)&&(vry_value<1650))&&(ultima_direcao!=3)){
                        atual_direcao = 4;
                    }
                }
            }
        }
        ultima_direcao = atual_direcao;

        while(ptr2!=NULL){
            ptr->parte[0]=ptr2->parte[0];
            ptr->parte[1]=ptr2->parte[1];

            ptr = ptr->next;
            ptr2 = ptr2->next;
        }
        ptr2 = corpo;
        while(ptr2->next != ptr){
            if(ptr->parte[0]==ptr2->parte[0] && ptr->parte[1]==ptr2->parte[1]){
                Jogando = false;
                snake_tempo_final = to_ms_since_boot(get_absolute_time());
            }
            ptr2 = ptr2->next;
        }
        ptr2 = corpo;

        switch(atual_direcao){
            case 1:
                ptr->parte[0] += 2;
                break;
            case 2:
                ptr->parte[0] -= 2;
                break;
            case 3:
                ptr->parte[1] -= 2;
                break;
            case 4:
                ptr->parte[1] += 2;
                break;
            default:
                break;
        }

        if(ptr->parte[0]==fruta[0] && ptr->parte[1]==fruta[1]){
            fruta[0] = ((rand()%15)*4)+2;
            fruta[1] = ((rand()%15)*4)+2;

            cobra *a = malloc(sizeof(cobra));
            a->parte[0]=ptr->parte[0];
            a->parte[1]=ptr->parte[1];

            a->next=corpo;

            corpo = a;
            contador++;

            if(contador==10){
                contador_dezena++;
                contador=0;
            }
            if(contador_dezena==10){
                contador_centena++;
                contador_dezena=0;
            }
        }

        while(Jogando==true&&ptr_checagem!=NULL){
            if(ptr_checagem->parte[0]==fruta[0]&&ptr_checagem->parte[1]==fruta[1]){
                repetidor();
            }
            ptr_checagem = ptr_checagem->next;
        }

        ssd1306_rect(&ssd, 2, 2, 60, 60, false, true);
        ssd1306_rect(&ssd, fruta[1],fruta[0], 2,2, true, true);
        while(Jogando==true&&ptr2!=NULL){
            ssd1306_rect(&ssd, ptr2->parte[1],ptr2->parte[0], 2,2, true, true);
            ptr2 = ptr2->next;
        }
        ssd1306_rect(&ssd, 34, 64, 64, 7, false, true);
        ssd1306_draw_char(&ssd, (char)contador_centena+48, 87, 34);
        ssd1306_draw_char(&ssd, (char)contador_dezena+48, 94, 34);
        ssd1306_draw_char(&ssd, (char)contador+48, 101, 34);
        ssd1306_send_data(&ssd);
        ptr2 = corpo->next;

        if((ptr->parte[0]==0 || ptr->parte[0]==62) || (ptr->parte[1]== 0 || ptr->parte[1]==62)){
            Jogando = false;
            snake_tempo_final = to_us_since_boot(get_absolute_time());
        }
        ptr = corpo;
        sleep_ms(100);
    }
    while(fim_de_jogo){
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "Score", 45, 20);
        ssd1306_draw_char(&ssd, (char)contador_centena+48, 55, 34);
        ssd1306_draw_char(&ssd, (char)contador_dezena+48, 62, 34);
        ssd1306_draw_char(&ssd, (char)contador+48, 69, 34);
        ssd1306_draw_string(&ssd, "Voltar", 38, 48);
        ssd1306_draw_char(&ssd, '<', 90, 48);
        ssd1306_send_data(&ssd);
    }
    fruta[0] = 42;
    fruta[1] = 30;

    atual_direcao = 0, ultima_direcao = 0, contador_centena = 0, contador_dezena = 0, contador = 0;
    corpo = NULL;
    ptr_checagem = NULL;
    fim_de_jogo = true;
    return (snake_tempo_final - snake_tempo_inicial);
}


//----------------------------------------------- JOGO ------------------------------------------------
//---------------------------------------------- Memoria ----------------------------------------------
uint32_t Memoria(){
    uint32_t memoria_tempo_inicial = to_ms_since_boot(get_absolute_time()), memoria_tempo_final = 0;

    int indice = 12, teste_memoria[3]={0,0,0};
    int matriz_memoria[25] = {0}, teste = 0, nivel = 1;
    int top = 25, left = 57;
    char *resultado = "Derrota";

    gpio_set_irq_enabled_with_callback(botao_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler_memoria);

    while(selecao_de_nivel){
        adc_select_input(0);
        uint16_t vry_value = adc_read();

        if((vry_value<=2732)&&(vry_value>=1368)){
            atual_direcao = 0;
        }else{
            if((ultima_direcao==0) && (vry_value>2732)){
                atual_direcao = -1;
            }else{
                if((ultima_direcao==0)&&(vry_value<1368)){
                    atual_direcao = 1;
                }
            }
        }

        if(((nivel > 1 && nivel < 7)||(nivel==1 && atual_direcao == 1)||(nivel==7 && atual_direcao == -1))&&(atual_direcao!=2)){
            nivel += atual_direcao;
        }
        switch(nivel){
            case 1:
                ssd1306_fill(&ssd, false);
                ssd1306_draw_string(&ssd, "Escolha", 42, 0);
                ssd1306_draw_char(&ssd, '<', 103, 35);
                ssd1306_draw_string(&ssd, "Level 1", 0, 35);
                ssd1306_draw_string(&ssd, "Level 2", 0, 50);
                ssd1306_send_data(&ssd);
                break;
                
            case 7:
                ssd1306_fill(&ssd, false);
                ssd1306_draw_string(&ssd, "Escolha", 42, 0);
                ssd1306_draw_char(&ssd, '<', 103, 35);
                ssd1306_draw_string(&ssd, "Level 6", 0, 20);
                ssd1306_draw_string(&ssd, "Level 7", 0, 35);
                ssd1306_send_data(&ssd);
                break;

            default:
                ssd1306_fill(&ssd, false);
                ssd1306_draw_string(&ssd, "Escolha", 42, 0);
                ssd1306_draw_char(&ssd, '<', 103, 35);
                ssd1306_draw_string(&ssd, "Level", 0, 20);
                ssd1306_draw_string(&ssd, "Level", 0, 35);
                ssd1306_draw_string(&ssd, "Level", 0, 50);

                ssd1306_draw_char(&ssd, (char)nivel+47, 49, 20);
                ssd1306_draw_char(&ssd, (char)nivel+48, 49, 35);
                ssd1306_draw_char(&ssd, (char)nivel+49, 49, 50);
                ssd1306_send_data(&ssd);
                break;
        }
        ultima_direcao = atual_direcao;
        atual_direcao=2;
    }
    ultima_direcao = 0, atual_direcao = 0;

    srand(time(NULL));

    for(int i = 0; i<nivel; i++){
        for(int j = 1; j<3; j++){

            int k = (rand()%25);
            if(matriz_memoria[k] == 0){
                matriz_memoria[k] = i+1;
            }else{
                j--;
            }
        }
    }

    for(int i = 0; i<25; i++){
        switch(matriz_memoria[i]){
            case 1:
                matriz[1][i][0] = 128;
                matriz[1][i][1] = 0;
                matriz[1][i][2] = 0;
                break;
            case 2:
                matriz[1][i][0] = 0;
                matriz[1][i][1] = 128;
                matriz[1][i][2] = 0;
                break;
            case 3:
                matriz[1][i][0] = 0;
                matriz[1][i][1] = 0;
                matriz[1][i][2] = 128;
                break;
            case 4:
                matriz[1][i][0] = 128;
                matriz[1][i][1] = 128;
                matriz[1][i][2] = 0;
                break;
            case 5:
                matriz[1][i][0] = 128;
                matriz[1][i][1] = 0;
                matriz[1][i][2] = 128;
                break;
            case 6:
                matriz[1][i][0] = 0;
                matriz[1][i][1] = 128;
                matriz[1][i][2] = 128;
                break;
            case 7:
                matriz[1][i][0] = 128;
                matriz[1][i][1] = 128;
                matriz[1][i][2] = 128;
                break;
        }
    }

    write(matriz[1]);
    sleep_ms(10);

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Iniciando", 25, 20);
    ssd1306_draw_string(&ssd, "em", 49, 27);

    for(int i = 5; i>0; i--){
        ssd1306_draw_char(&ssd, (i+48), 56, 40);
        ssd1306_send_data(&ssd);
        sleep_ms(1000);
        ssd1306_rect(&ssd, 34, 69, 7, 7, false, true);
    }

    write(matriz[0]);
    sleep_ms(10);
    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 41, 45, 4, 4, true, true);
    ssd1306_rect(&ssd, 41, 52, 4, 4, true, true);
    ssd1306_rect(&ssd, 41, 59, 4, 4, true, true);
    ssd1306_rect(&ssd, 41, 66, 4, 4, true, true);
    ssd1306_rect(&ssd, 41, 73, 4, 4, true, true);

    ssd1306_rect(&ssd, 34, 45, 4, 4, true, true);
    ssd1306_rect(&ssd, 34, 52, 4, 4, true, true);
    ssd1306_rect(&ssd, 34, 59, 4, 4, true, true);
    ssd1306_rect(&ssd, 34, 66, 4, 4, true, true);
    ssd1306_rect(&ssd, 34, 73, 4, 4, true, true);

    ssd1306_rect(&ssd, 27, 45, 4, 4, true, true);
    ssd1306_rect(&ssd, 27, 52, 4, 4, true, true);
    ssd1306_rect(&ssd, 27, 59, 4, 4, true, true);
    ssd1306_rect(&ssd, 27, 66, 4, 4, true, true);
    ssd1306_rect(&ssd, 27, 73, 4, 4, true, true);

    ssd1306_rect(&ssd, 20, 45, 4, 4, true, true);
    ssd1306_rect(&ssd, 20, 52, 4, 4, true, true);
    ssd1306_rect(&ssd, 20, 59, 4, 4, true, true);
    ssd1306_rect(&ssd, 20, 66, 4, 4, true, true);
    ssd1306_rect(&ssd, 20, 73, 4, 4, true, true);

    ssd1306_rect(&ssd, 13, 45, 4, 4, true, true);
    ssd1306_rect(&ssd, 13, 52, 4, 4, true, true);
    ssd1306_rect(&ssd, 13, 59, 4, 4, true, true);
    ssd1306_rect(&ssd, 13, 66, 4, 4, true, true);
    ssd1306_rect(&ssd, 13, 73, 4, 4, true, true);

    ssd1306_rect(&ssd, top, left, 8, 8, true, false);
    ssd1306_send_data(&ssd);

    gpio_set_irq_enabled_with_callback(botao_b, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler_memoria);

    while(Jogando){
        adc_select_input(0);
        uint16_t vry_value = adc_read();
        adc_select_input(1);
        uint16_t vrx_value = adc_read();
        
        if(((vry_value<=2250&&vry_value>=1850)&&(vrx_value<=2250))&&(vrx_value>=1850)){
            atual_direcao = 0;
        }else{
            if(((vry_value<=2250&&vry_value>=1850)&&(vrx_value>2250))&&(ultima_direcao==0)){
                atual_direcao = 1;
            }else{
                if(((vry_value<=2250&&vry_value>=1850)&&(vrx_value<1850))&&(ultima_direcao==0)){
                    atual_direcao = 2;
                }else{
                    if(((vrx_value<=2250&&vrx_value>=1850)&&(vry_value>2250))&&(ultima_direcao==0)){
                        atual_direcao = 3;
                    }else{
                        if(((vrx_value<=2250&&vrx_value>=1850)&&(vry_value<1850))&&(ultima_direcao==0)){
                            atual_direcao = 4;
                        }
                    }
                }
            }
        }

        ssd1306_rect(&ssd, top, left, 8, 8, false, false);
        if(ultima_direcao==0){
            switch(atual_direcao){
                case 1:
                    if(left<70){
                        left += 7;
                        indice++;
                    }
                    break;
    
                case 2:
                    if(left>43){
                        left -= 7;
                        indice--;
                    }
                    break;
    
                case 3:
                    if(top>12){
                        top -= 7;
                        indice -= 5;
                    }
                    break;
                case 4:
                    if(top<39){
                        top += 7;
                        indice += 5;
                    }
                    break;
    
            }
        }

        if(verificar){
            if((matriz[1][indice][0] == 0 && matriz[1][indice][1] == 0) && matriz[1][indice][2] == 0){
                Jogando = false;
                memoria_tempo_final = to_ms_since_boot(get_absolute_time());
                teste_memoria[0] = 0;
                teste_memoria[1] = 0;
                teste_memoria[2] = 0;
            }else{
                if((teste_memoria[0]!=0 || teste_memoria[1]!=0) || teste_memoria[2]!=0){
                    if((teste_memoria[0]==matriz[1][indice][0] && teste_memoria[1]==matriz[1][indice][1]) && teste_memoria[2]==matriz[1][indice][2]){
                        for(int i = 0; i<3; i++){
                            matriz[0][indice][i] = matriz[1][indice][i];
                        }
                        write(matriz[0]);
                        sleep_ms(10);
                    }else{
                        Jogando = false;
                        memoria_tempo_final = to_us_since_boot(get_absolute_time());
                    }
        
                    teste_memoria[0] = 0;
                    teste_memoria[1] = 0;
                    teste_memoria[2] = 0;
                }else{
                    if((matriz[0][indice][0]==0&&matriz[0][indice][1]==0)&& matriz[0][indice][2]==0){
                        for(int i = 0; i<3; i++){
                            matriz[0][indice][i] = matriz[1][indice][i];
                            teste_memoria[i] = matriz[0][indice][i];
                        }
                        write(matriz[0]);
                        sleep_ms(10);
                    }
                }
            }
            verificar = false;
        }





        
        ssd1306_rect(&ssd, top, left, 8, 8, true, false);
        ssd1306_send_data(&ssd);
        for(int i = 0; i<25; i++){
            for(int j = 0; j<3; j++){
                if(matriz[1][i][j]!=matriz[0][i][j]){
                    break;
                }
                teste++;
            }
        }
        if(teste==75){
            resultado = "Vitoria";
            Jogando = false;
            memoria_tempo_final = to_ms_since_boot(get_absolute_time());
            
        }
        teste = 0;
        ultima_direcao = atual_direcao;
    }
    while(fim_de_jogo){
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, resultado, 35, 20);
        ssd1306_draw_string(&ssd, "Voltar", 38, 48);
        ssd1306_draw_char(&ssd, '<', 90, 48);
        ssd1306_send_data(&ssd);
    }

    for(int i = 0; i<25; i++){
        for(int j = 0; j<3; j++){
            matriz[0][i][j] = 0;
            matriz[1][i][j] = 0;
        }
    }
    write(matriz[0]);

    atual_direcao = 0, ultima_direcao = 0;

    fim_de_jogo = true, selecao_de_nivel = true;

    return (memoria_tempo_final - memoria_tempo_inicial);
}


//----------------------------------------------- JOGO ------------------------------------------------
//----------------------------------------------- Pong ------------------------------------------------
uint32_t Pong(){
    uint32_t pong_tempo_inicial = to_ms_since_boot(get_absolute_time()), pong_tempo_final = 0;

    int top=0, x=30, y=30, step_x=2, step_y=2;

    gpio_set_irq_enabled_with_callback(botao_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler_snake);

    while(Jogando){
        ssd1306_fill(&ssd, false);
        ssd1306_rect(&ssd, 0, 77, 4, 64, true, true);
        ssd1306_rect(&ssd, top, 0, 4, 16, true, true);
        ssd1306_rect(&ssd, y, x, 4, 4, true, true);
        ssd1306_draw_string(&ssd, "Score", 85, 20);
        ssd1306_draw_char(&ssd, (char)contador_centena+48, 95, 34);
        ssd1306_draw_char(&ssd, (char)contador_dezena+48, 102, 34);
        ssd1306_draw_char(&ssd, (char)contador+48, 109, 34);
        ssd1306_send_data(&ssd);

        adc_select_input(0);
        uint16_t vry_value = adc_read();
        
        if(vry_value<=2250&&vry_value>=1850){
            atual_direcao = 0;
        }else{
            if(vry_value>2250){
                atual_direcao = -1;
            }else{
                if(vry_value<1850){
                    atual_direcao = 1;
                }
            }
        }

        switch(atual_direcao){
            case -1:
                if(top!=0){
                    top-=2;
                }
                break;

            case 1:
                if(top!=46){
                    top+=2;
                }
                break;
        }

        if(x>=73){
            step_x = -2;
        }
            
        if((y<=0)||(y>=59)){
            step_y = (-1)*step_y;
        }

        if((x==4||x==3)&&(y>=(top-3)&&y<=(top+15))){
            step_x = 2;
            if(y<=(top+5)){
                if(step_y>0){
                    step_y = -2;
                }
            }else{
                if(y>(top+9)){
                    if(step_y<0){
                        step_y = 2;
                    }
                }
            }
            contador++;
            if(contador==10){
                contador_dezena++;
                contador=0;
            }
            if(contador_dezena==10){
                contador_centena++;
                contador_dezena=0;
            }
        }

        if(x<=0){
            Jogando=false;
            pong_tempo_final = to_ms_since_boot(get_absolute_time());
        }

        x+=step_x;
        y+=step_y;

        sleep_ms(10);
    }
    while(fim_de_jogo){
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "Score", 45, 20);
        ssd1306_draw_char(&ssd, (char)contador_centena+48, 55, 34);
        ssd1306_draw_char(&ssd, (char)contador_dezena+48, 62, 34);
        ssd1306_draw_char(&ssd, (char)contador+48, 69, 34);
        ssd1306_draw_string(&ssd, "Voltar", 38, 48);
        ssd1306_draw_char(&ssd, '<', 90, 48);
        ssd1306_send_data(&ssd);
    }
    atual_direcao = 0, contador_centena = 0, contador_dezena = 0, contador = 0;
    fim_de_jogo = true;

    return (pong_tempo_final - pong_tempo_inicial);
}


//------------------------------------------ Função Principal -----------------------------------------
int main(){
//--------------------------------------- Inicialização de GPIO ---------------------------------------
    int jogo_selecionado = 0, relatorio_selecionado = 0;
    uint32_t tempo[5] = {0,0,0,0,0};

    char lista_de_jogos[] = "Snake\0Memoria\0Pong\0Vazio\0Vazio";
    int index_jogos[] = {0,6,14,19,25};

    stdio_init_all();

    pixel_init(pixel);
    
    gpio_init(botao_a);
    gpio_set_dir(botao_a, GPIO_IN);
    gpio_pull_up(botao_a);

    gpio_init(botao_b);
    gpio_set_dir(botao_b, GPIO_IN);
    gpio_pull_up(botao_b);

    gpio_init(botao_joy);
    gpio_set_dir(botao_joy, GPIO_IN);
    gpio_pull_up(botao_joy);
    
//-------------------------------------- Inicialização do Display -------------------------------------
    // Inicialização do I2C. Utilizando a 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configurando GPIO para I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configurando GPIO para I2C
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    ssd1306_draw_string(&ssd, "GAMING FRIEND", 11, 7);
    ssd1306_draw_string(&ssd, "Aperte A", 28, 34);
    ssd1306_draw_string(&ssd, "para continuar", 7, 48);
    ssd1306_send_data(&ssd);

//---------------------------------------- Inicialização do ADC ---------------------------------------
    adc_init(); //Inicializa o módulo ADC

    adc_gpio_init(eixo_x); //Configura a porta 26 para leitura analógica do ADC.

    adc_gpio_init(eixo_y); //Configura a porta 27 para leitura analógica do ADC.


//------------------------------------- Configuração de interrupção -----------------------------------
    gpio_set_irq_enabled_with_callback(botao_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(botao_joy, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

//------------------------------------------- Loop Principal ------------------------------------------
    while(true){
        while(Iniciar){
            adc_select_input(0);
            uint16_t vry_value = adc_read();

            if((vry_value<=2732)&&(vry_value>=1368)){
                atual_direcao = 0;
            }else{
                if((ultima_direcao==0) && (vry_value>2732)){
                    atual_direcao = -1;
                }else{
                    if((ultima_direcao==0)&&(vry_value<1368)){
                        atual_direcao = 1;
                    }
                }
            }

            if(((jogo_selecionado > 0 && jogo_selecionado < 4)||(jogo_selecionado==0 && atual_direcao == 1)||(jogo_selecionado==4 && atual_direcao == -1))&&(atual_direcao!=2)){
                jogo_selecionado += atual_direcao;
            }
            switch(jogo_selecionado){
                case 0:
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_string(&ssd, "MENU", 42, 0);
                    ssd1306_draw_char(&ssd, '<', 103, 35);
                    ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[jogo_selecionado]], 0, 35);
                    ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[jogo_selecionado+1]], 0, 50);
                    ssd1306_send_data(&ssd);
                    break;
                case 4:
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_string(&ssd, "MENU", 42, 0);
                    ssd1306_draw_char(&ssd, '<', 103, 35);
                    ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[jogo_selecionado-1]], 0, 20);
                    ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[jogo_selecionado]], 0, 35);
                    ssd1306_send_data(&ssd);
                    break;
                default:
                    ssd1306_fill(&ssd, false);
                    ssd1306_draw_string(&ssd, "MENU", 42, 0);
                    ssd1306_draw_char(&ssd, '<', 103, 35);
                    ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[jogo_selecionado-1]], 0, 20);
                    ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[jogo_selecionado]], 0, 35);
                    ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[jogo_selecionado+1]], 0, 50);
                    ssd1306_send_data(&ssd);
                    break;
            }
            ultima_direcao = atual_direcao;
            atual_direcao=2;

            if(Jogando){
                switch(jogo_selecionado){
                    case 0:
                        tempo[0] += Snake();
                        gpio_set_irq_enabled_with_callback(botao_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
                        break;
                    case 1:
                        tempo[1] += Memoria();
                        gpio_set_irq_enabled_with_callback(botao_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
                        break;
                    case 2:
                        tempo[2] += Pong();
                        gpio_set_irq_enabled_with_callback(botao_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
                        break;
                    case 3:
                        //Vazio();
                        Jogando=false;
                        break;
                    case 4:
                        //Vazio();
                        Jogando=false;
                        break;
                }
            }

            while(Relatorio){
                adc_select_input(0);
                uint16_t vry_value = adc_read();
            
                if((vry_value<=2732)&&(vry_value>=1368)){
                    atual_direcao = 0;
                }else{
                    if((ultima_direcao==0) && (vry_value>2732)){
                        atual_direcao = -1;
                    }else{
                        if((ultima_direcao==0)&&(vry_value<1368)){
                            atual_direcao = 1;
                        }
                    }
                }
            
                if(((relatorio_selecionado > 0 && relatorio_selecionado < 4)||(relatorio_selecionado==0 && atual_direcao == 1)||(relatorio_selecionado==4 && atual_direcao == -1))&&(atual_direcao!=2)){
                    relatorio_selecionado += atual_direcao;
                }
                switch(relatorio_selecionado){
                    case 0:
                        ssd1306_fill(&ssd, false);
                        ssd1306_draw_string(&ssd, "RELATORIO", 25, 0);
                        ssd1306_draw_string(&ssd, "H  M  S", 71, 10);
                        ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[relatorio_selecionado]], 0, 35);
                        ssd1306_draw_char(&ssd, (char)((tempo[relatorio_selecionado]/1000)/36000)+48, 70, 42);
                        ssd1306_draw_char(&ssd, (char)(((tempo[relatorio_selecionado]/1000)%36000)/3600)+48, 77, 42);
                        ssd1306_draw_char(&ssd, (char)((((tempo[relatorio_selecionado]/1000)%36000)%3600)/600)+48, 91, 42);
                        ssd1306_draw_char(&ssd, (char)(((((tempo[relatorio_selecionado]/1000)%36000)%3600)%600)/60)+48, 98, 42);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado]/1000)%36000)%3600)%600)%60)/10)+48, 112, 42);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado]/1000)%36000)%3600)%600)%60)%10)+48, 119, 42);
            
                        ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[relatorio_selecionado+1]], 0, 50);
                        ssd1306_draw_char(&ssd, (char)((tempo[relatorio_selecionado+1]/1000)/36000)+48, 70, 57);
                        ssd1306_draw_char(&ssd, (char)(((tempo[relatorio_selecionado+1]/1000)%36000)/3600)+48, 77, 57);
                        ssd1306_draw_char(&ssd, (char)((((tempo[relatorio_selecionado+1]/1000)%36000)%3600)/600)+48, 91, 57);
                        ssd1306_draw_char(&ssd, (char)(((((tempo[relatorio_selecionado+1]/1000)%36000)%3600)%600)/60)+48, 98, 57);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado+1]/1000)%36000)%3600)%600)%60)/10)+48, 112, 57);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado+1]/1000)%36000)%3600)%600)%60)%10)+48, 119, 57);
                        ssd1306_send_data(&ssd);
                        break;

                    case 4:
                        ssd1306_fill(&ssd, false);
                        ssd1306_draw_string(&ssd, "RELATORIO", 25, 0);
                        ssd1306_draw_string(&ssd, "H  M  S", 71, 10);
                        ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[relatorio_selecionado]], 0, 35);
                        ssd1306_draw_char(&ssd, (char)((tempo[relatorio_selecionado]/1000)/36000)+48, 71, 42);
                        ssd1306_draw_char(&ssd, (char)(((tempo[relatorio_selecionado]/1000)%36000)/3600)+48, 78, 42);
                        ssd1306_draw_char(&ssd, (char)((((tempo[relatorio_selecionado]/1000)%36000)%3600)/600)+48, 92, 42);
                        ssd1306_draw_char(&ssd, (char)(((((tempo[relatorio_selecionado]/1000)%36000)%3600)%600)/60)+48, 99, 42);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado]/1000)%36000)%3600)%600)%60)/10)+48, 113, 42);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado]/1000)%36000)%3600)%600)%60)%10)+48, 120, 42);
            
                        ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[relatorio_selecionado-1]], 0, 20);
                        ssd1306_draw_char(&ssd, (char)((tempo[relatorio_selecionado-1]/1000)/36000)+48, 71, 27);
                        ssd1306_draw_char(&ssd, (char)(((tempo[relatorio_selecionado-1]/1000)%36000)/3600)+48, 78, 27);
                        ssd1306_draw_char(&ssd, (char)((((tempo[relatorio_selecionado-1]/1000)%36000)%3600)/600)+48, 92, 27);
                        ssd1306_draw_char(&ssd, (char)(((((tempo[relatorio_selecionado-1]/1000)%36000)%3600)%600)/60)+48, 99, 27);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado-1]/1000)%36000)%3600)%600)%60)/10)+48, 113, 27);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado-1]/1000)%36000)%3600)%600)%60)%10)+48, 120, 27);
                        ssd1306_send_data(&ssd);
                        printf("Teste\n");
                        break;

                    default:
                        ssd1306_fill(&ssd, false);
                        ssd1306_draw_string(&ssd, "RELATORIO", 25, 0);
                        ssd1306_draw_string(&ssd, "H  M  S", 71, 10);
                        ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[relatorio_selecionado+1]], 0, 50);
                        ssd1306_draw_char(&ssd, (char)((tempo[relatorio_selecionado+1]/1000)/36000)+48, 71, 57);
                        ssd1306_draw_char(&ssd, (char)(((tempo[relatorio_selecionado+1]/1000)%36000)/3600)+48, 78, 57);
                        ssd1306_draw_char(&ssd, (char)((((tempo[relatorio_selecionado+1]/1000)%36000)%3600)/600)+48, 92, 57);
                        ssd1306_draw_char(&ssd, (char)(((((tempo[relatorio_selecionado+1]/1000)%36000)%3600)%600)/60)+48, 99, 57);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado+1]/1000)%36000)%3600)%600)%60)/10)+48, 113, 57);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado+1]/1000)%36000)%3600)%600)%60)%10)+48, 120, 57);
            
                        ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[relatorio_selecionado]], 0, 35);
                        ssd1306_draw_char(&ssd, (char)((tempo[relatorio_selecionado]/1000)/36000)+48, 71, 42);
                        ssd1306_draw_char(&ssd, (char)(((tempo[relatorio_selecionado]/1000)%36000)/3600)+48, 78, 42);
                        ssd1306_draw_char(&ssd, (char)((((tempo[relatorio_selecionado]/1000)%36000)%3600)/600)+48, 92, 42);
                        ssd1306_draw_char(&ssd, (char)(((((tempo[relatorio_selecionado]/1000)%36000)%3600)%600)/60)+48, 99, 42);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado]/1000)%36000)%3600)%600)%60)/10)+48, 113, 42);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado]/1000)%36000)%3600)%600)%60)%10)+48, 120, 42);
            
                        ssd1306_draw_string(&ssd, &lista_de_jogos[index_jogos[relatorio_selecionado-1]], 0, 20);
                        ssd1306_draw_char(&ssd, (char)((tempo[relatorio_selecionado-1]/1000)/36000)+48, 71, 27);
                        ssd1306_draw_char(&ssd, (char)(((tempo[relatorio_selecionado-1]/1000)%36000)/3600)+48, 78, 27);
                        ssd1306_draw_char(&ssd, (char)((((tempo[relatorio_selecionado-1]/1000)%36000)%3600)/600)+48, 92, 27);
                        ssd1306_draw_char(&ssd, (char)(((((tempo[relatorio_selecionado-1]/1000)%36000)%3600)%600)/60)+48, 99, 27);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado-1]/1000)%36000)%3600)%600)%60)/10)+48, 113, 27);
                        ssd1306_draw_char(&ssd, (char)((((((tempo[relatorio_selecionado-1]/1000)%36000)%3600)%600)%60)%10)+48, 120, 27);
                        ssd1306_send_data(&ssd);
                        break;
                }
                ultima_direcao = atual_direcao;
                atual_direcao = 2;
                sleep_ms(1);
            }
        }
        sleep_ms(1);
    }
}