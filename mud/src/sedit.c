/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
*    				                                          *
*  OasisOLC - sedit.c 		                                          *
*    				                                          *
*  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "olc.h"

/*-------------------------------------------------------------------*/
/* external variables */
extern struct shop_data *shop_index;		/*. shop.c	.*/
extern int top_shop;				/*. shop.c	.*/
extern struct char_data *mob_proto;		/*. db.c	.*/
extern struct obj_data *obj_proto;		/*. db.c	.*/
extern struct room_data *world;			/*. db.c	.*/
extern struct zone_data *zone_table;		/*. db.c	.*/
extern struct index_data *mob_index;		/*. db.c	.*/
extern struct index_data *obj_index;		/*. db.c	.*/
extern char *trade_letters[];			/*. shop.h	.*/
extern char *shop_bits[];			/*. shop.h	.*/
extern char *item_types[];			/*. constants.c	.*/

/*-------------------------------------------------------------------*/
/*. Handy  macros .*/

#define S_NUM(i)		((i)->virtual)
#define S_KEEPER(i)		((i)->keeper)
#define S_OPEN1(i)		((i)->open1)
#define S_CLOSE1(i)		((i)->close1)
#define S_OPEN2(i)		((i)->open2)
#define S_CLOSE2(i)		((i)->close2)
#define S_BANK(i)		((i)->bankAccount)
#define S_BROKE_TEMPER(i)	((i)->temper1)
#define S_BITVECTOR(i)		((i)->bitvector)
#define S_NOTRADE(i)		((i)->with_who)
#define S_SORT(i)		((i)->lastsort)
#define S_BUYPROFIT(i)		((i)->profit_buy)
#define S_SELLPROFIT(i)		((i)->profit_sell)
#define S_FUNC(i)		((i)->func)

#define S_ROOMS(i)		((i)->in_room)
#define S_PRODUCTS(i)		((i)->producing)
#define S_NAMELISTS(i)		((i)->type)
#define S_ROOM(i, num)		((i)->in_room[(num)])
#define S_PRODUCT(i, num)	((i)->producing[(num)])
#define S_BUYTYPE(i, num)	(BUY_TYPE((i)->type[(num)]))
#define S_BUYWORD(i, num)	(BUY_WORD((i)->type[(num)]))

#define S_NOITEM1(i)		((i)->no_such_item1)
#define S_NOITEM2(i)		((i)->no_such_item2)
#define S_NOCASH1(i)		((i)->missing_cash1)
#define S_NOCASH2(i)		((i)->missing_cash2)
#define S_NOBUY(i)		((i)->do_not_buy)
#define S_BUY(i)		((i)->message_buy)
#define S_SELL(i)		((i)->message_sell)

/*-------------------------------------------------------------------*/
/*. Function prototypes .*/

int real_shop(int vshop_num);
void sedit_setup_new(struct descriptor_data *d);
void sedit_setup_existing(struct descriptor_data *d, int rmob_num);
void sedit_parse(struct descriptor_data * d, char *arg);
void sedit_disp_menu(struct descriptor_data * d);
void sedit_namelist_menu(struct descriptor_data *d);
void sedit_types_menu(struct descriptor_data *d);
void sedit_products_menu(struct descriptor_data *d);
void sedit_rooms_menu(struct descriptor_data *d);
void sedit_compact_rooms_menu(struct descriptor_data *d);
void sedit_shop_flags_menu(struct descriptor_data *d);
void sedit_no_trade_menu(struct descriptor_data *d);
void sedit_save_internally(struct descriptor_data *d);
void sedit_save_to_disk(struct descriptor_data *d);
void copy_shop(struct shop_data *tshop, struct shop_data *fshop);
void copy_list(int **tlist, int *flist);
void copy_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist);
void sedit_add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *new);
void sedit_remove_from_type_list(struct shop_buy_data **list, int num);
void free_shop_strings(struct shop_data *shop);
void free_type_list(struct shop_buy_data **list);
void free_shop(struct shop_data *shop);
void sedit_modify_string(char **str, char *new);

/*. External .*/
SPECIAL(shop_keeper);

/*-------------------------------------------------------------------*\
  utility functions 
\*-------------------------------------------------------------------*/

void sedit_setup_new(struct descriptor_data *d)
{
  /* No shop data - create some dummy data to start with */
  CREATE(OLC_SHOP(d), struct shop_data, 1);

  OLC_SHOP(d)->keeper = -1;
  OLC_SHOP(d)->no_such_item1 = str_dup("Sorry, I don't stock that item.");
  OLC_SHOP(d)->no_such_item2 = str_dup("You don't seem to have that.");
  OLC_SHOP(d)->missing_cash1 = str_dup("I can't afford that!");
  OLC_SHOP(d)->missing_cash2 = str_dup("You are too poor!");
  OLC_SHOP(d)->do_not_buy = str_dup("I don't trade in such items.");
  OLC_SHOP(d)->message_buy = str_dup("That'll be %d coins, thanks.");
  OLC_SHOP(d)->message_sell = str_dup("I'll give you %d coins for that.");
  OLC_SHOP(d)->temper1 = 0;
  OLC_SHOP(d)->bitvector = 0;
  OLC_SHOP(d)->keeper = 0;
  OLC_SHOP(d)->with_who = 0;
  OLC_SHOP(d)->in_room = 0;
  OLC_SHOP(d)->open1 = 0;
  OLC_SHOP(d)->close1 = 28;
  OLC_SHOP(d)->open2 = 0;
  OLC_SHOP(d)->close2 = 28;
}

/*-------------------------------------------------------------------*/

void sedit_setup_existing(struct descriptor_data *d, int rshop_num)
{ 
  /*. Alloc some shop shaped space .*/
  CREATE(OLC_SHOP(d), struct shop_data, 1);
  copy_shop(OLC_SHOP(d), shop_index + rshop_num);
  sedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void copy_shop(struct shop_data *tshop, struct shop_data *fshop)
{
  /*. Copy basic info over .*/
  S_NUM(tshop) = S_NUM(fshop); 
  S_KEEPER(tshop) = S_KEEPER(fshop);
  S_OPEN1(tshop) = S_OPEN1(fshop);
  S_CLOSE1(tshop) = S_CLOSE1(fshop);
  S_OPEN2(tshop) = S_OPEN2(fshop);
  S_CLOSE2(tshop) = S_CLOSE2(fshop);
  S_BANK(tshop) = S_BANK(fshop);
  S_BROKE_TEMPER(tshop) = S_BROKE_TEMPER(fshop);
  S_BITVECTOR(tshop) = S_BITVECTOR(fshop);
  S_NOTRADE(tshop) = S_NOTRADE(fshop);
  S_SORT(tshop) = S_SORT(fshop);
  S_BUYPROFIT(tshop) = S_BUYPROFIT(fshop);
  S_SELLPROFIT(tshop) = S_SELLPROFIT(fshop);
  S_FUNC(tshop) = S_FUNC(fshop);
 
  /*. Copy lists over .*/ 
  copy_list(&(S_ROOMS(tshop)), S_ROOMS(fshop));
  copy_list(&(S_PRODUCTS(tshop)), S_PRODUCTS(fshop));
  copy_type_list(&(tshop->type), fshop->type);

  /*. Copy notification strings over .*/
  free_shop_strings(tshop);
  S_NOITEM1(tshop) = str_dup(S_NOITEM1(fshop));
  S_NOITEM2(tshop) = str_dup(S_NOITEM2(fshop));
  S_NOCASH1(tshop) = str_dup(S_NOCASH1(fshop));
  S_NOCASH2(tshop) = str_dup(S_NOCASH2(fshop));
  S_NOBUY(tshop)   = str_dup(S_NOBUY(fshop));
  S_BUY(tshop)     = str_dup(S_BUY(fshop));
  S_SELL(tshop)    = str_dup(S_SELL(fshop));

}

/*-------------------------------------------------------------------*/
/*. Copy a -1 terminated integer array list .*/

void copy_list(int **tlist, int *flist)
{ int num_items, i;
  
  if (*tlist)
    free(*tlist);
 
  /*. Count number of entries .*/
  for (i=0; flist[i] != -1; i++)
    ;
  num_items = i + 1;
  
  /*. Make space for entries .*/
  CREATE(*tlist, int, num_items);

  /*. Copy entries over .*/
  i = 0;
  do
  { (*tlist)[i] = flist[i];
    i++;
  } while(i < num_items); 
}


/*-------------------------------------------------------------------*/
/*. Copy a -1 terminated (in the type field) shop_buy_data 
    array list .*/

void copy_type_list(struct shop_buy_data **tlist, struct shop_buy_data *flist)
{ int num_items, i;

  if (*tlist)
    free_type_list(tlist);
 
  /*. Count number of entries .*/
  for (i=0; BUY_TYPE(flist[i]) != -1; i++)
    ;
  num_items = i + 1;
  
  /*. Make space for entries .*/
  CREATE(*tlist, struct shop_buy_data, num_items);

  /*. Copy entries over .*/
  i = 0;
  do
  { (*tlist)[i].type = flist[i].type;
    if (BUY_WORD(flist[i]))
      BUY_WORD((*tlist)[i]) = str_dup(BUY_WORD(flist[i]));
    i++;
  } while(i < num_items); 
}

/*-------------------------------------------------------------------*/


void sedit_remove_from_type_list(struct shop_buy_data **list, int num)
{ int i, num_items;
  struct shop_buy_data *nlist;

  /*. Count number of entries .*/
  for (i=0; (*list)[i].type != -1; i++)
    ;

  if (num >= i || num < 0)
    return;
  num_items = i;
  
  CREATE(nlist, struct shop_buy_data, num_items);

  for(i = 0; i < num_items; i++)
    if (i < num)
      nlist[i] = (*list)[i];
    else
      nlist[i] = (*list)[i+1];

  free(BUY_WORD((*list)[num]));
  free(*list);
  *list = nlist;
}
 
/*-------------------------------------------------------------------*/

void sedit_add_to_type_list(struct shop_buy_data **list, struct shop_buy_data *new)
{ int i, num_items;
  struct shop_buy_data *nlist;

  /*. Count number of entries .*/
  for (i=0; (*list)[i].type != -1; i++)
    ;
  num_items = i;

  /*. make a new list and slot in the new entry .*/
  CREATE(nlist, struct shop_buy_data, num_items + 2);
  for(i = 0; i < num_items; i++)
    nlist[i] = (*list)[i];
  nlist[num_items] = *new;
  nlist[num_items+1].type = -1;

  /*. Out with the old, in with the new .*/
  free(*list);
  *list = nlist;
}

/*-------------------------------------------------------------------*/

void sedit_add_to_int_list(int **list, int new)
{ int i, num_items, *nlist;

  /*. Count number of entries .*/
  for (i=0; (*list)[i] != -1; i++)
    ;
  num_items = i;

  /*. make a new list and slot in the new entry .*/
  CREATE(nlist, int, num_items + 2);
  for(i = 0; i < num_items; i++)
    nlist[i] = (*list)[i];
  nlist[num_items] = new;
  nlist[num_items+1] = -1;

  /*. Out with the old, in with the new .*/
  free(*list);
  *list = nlist;
}

/*-------------------------------------------------------------------*/

void sedit_remove_from_int_list(int **list, int num)
{ int i, num_items, *nlist;

  /*. Count number of entries .*/
  for (i=0; (*list)[i] != -1; i++)
    ;

  if (num >= i || num < 0)
    return;
  num_items = i;
  
  CREATE(nlist, int, num_items);

  for(i = 0; i < num_items; i++)
    if (i < num)
      nlist[i] = (*list)[i];
    else
      nlist[i] = (*list)[i+1];

  free(*list);
  *list = nlist;
}
 
/*-------------------------------------------------------------------*/
/*. Free all the notice character strings in a shop structure .*/

void free_shop_strings(struct shop_data *shop)
{
  if (S_NOITEM1(shop))
  { free(S_NOITEM1(shop));
    S_NOITEM1(shop) = NULL;
  }
  if (S_NOITEM2(shop))
  { free(S_NOITEM2(shop));
    S_NOITEM2(shop) = NULL;
  }
  if (S_NOCASH1(shop))
  { free(S_NOCASH1(shop));
    S_NOCASH1(shop) = NULL;
  }
  if (S_NOCASH2(shop))
  { free(S_NOCASH2(shop));
    S_NOCASH2(shop) = NULL;
  }
  if (S_NOBUY(shop))
  { free(S_NOBUY(shop));
    S_NOBUY(shop) = NULL;
  }
  if (S_BUY(shop))
  { free(S_BUY(shop));
    S_BUY(shop) = NULL;
  }
  if (S_SELL(shop))
  { free(S_SELL(shop));
    S_SELL(shop) = NULL;
  }
}

/*-------------------------------------------------------------------*/
/*. Free a type list and all the strings it contains .*/

void free_type_list(struct shop_buy_data **list)
{ int i;
 
  for (i=0; (*list)[i].type != -1; i++)
    if (BUY_WORD((*list)[i]))
      free(BUY_WORD((*list)[i]));
  free(*list);
  *list = NULL;
}

/*-------------------------------------------------------------------*/
/*. Free up the whole shop structure and it's content .*/

void free_shop(struct shop_data *shop)
{ free_shop_strings(shop);
  free_type_list(&(S_NAMELISTS(shop)));
  free(S_ROOMS(shop));
  free(S_PRODUCTS(shop));
  free(shop);
}

/*-------------------------------------------------------------------*/

int real_shop(int vshop_num)
{ int rshop_num;

  for (rshop_num = 0; rshop_num < top_shop; rshop_num++)
    if (SHOP_NUM(rshop_num) == vshop_num)
      return rshop_num;

  return -1;
}

/*-------------------------------------------------------------------*/
/*. Generic string modifyer for shop keeper messages .*/

void sedit_modify_string(char **str, char *new)
{  char *pointer;

   /*. Check the '%s' is present, if not, add it .*/
   if (*new != '%')
   {  strcpy(buf, "%s ");
      strcat(buf, new);
      pointer = buf;
   } else
      pointer = new;

   if(*str)
     free(*str);
   *str = str_dup(pointer);
}

/*-------------------------------------------------------------------*/

void sedit_save_internally(struct descriptor_data *d)
{ int rshop, found = 0;
  struct shop_data *shop;
  struct shop_data *new_index;

  rshop = real_shop(OLC_NUM(d));
  shop = OLC_SHOP(d);
  S_NUM(shop) = OLC_NUM(d);

  if(rshop > -1)
  { /*. The shop already exists, just update it.*/
    copy_shop((shop_index + rshop), shop);
  } else
  { /*. Doesn't exist - hafta insert it .*/
    CREATE(new_index, struct shop_data, top_shop + 1);
    for(rshop = 0; rshop < top_shop; rshop++)
    { if(!found)
      { /*. Is this the place ?.*/
        if(SHOP_NUM(rshop) > OLC_NUM(d))
        { /*. Yep, stick it in here .*/
          found = 1;
          copy_shop(&(new_index[rshop]), shop);
          /*. Move the entry that used to go here up a place .*/
          new_index[rshop+1] = shop_index[rshop];
        } else
        { /*. This isn't the place, copy over info .*/
          new_index[rshop] = shop_index[rshop];
        }
      } else
      { /*. Shop's already inserted, copy rest over .*/
        new_index[rshop+1] = shop_index[rshop];
      }
    }
    if (!found)
      copy_shop(&(new_index[rshop]), shop);

    /*. Switch index in .*/
    free(shop_index);
    shop_index = new_index;
    top_shop++;
  }
  olc_add_to_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_SHOP);
}

        
/*-------------------------------------------------------------------*/

void sedit_save_to_disk(struct descriptor_data *d)
{ 
  int i, j, rshop, zone, top;
  FILE *shop_file;
  char fname[64];

  zone = zone_table[OLC_ZNUM(d)].number;
  top = zone_table[OLC_ZNUM(d)].top;

  sprintf(fname, "%s/%i.shp", SHP_PREFIX, zone);

  if(!(shop_file = fopen(fname, "w")))
  {  mudlog("SYSERR: OLC: Cannot open shop file!", BRF, LVL_BUILDER, TRUE);
     return;
  }
  if (fprintf(shop_file, "CircleMUD v3.0 Shop File~\n") < 0)
  {  mudlog("SYSERR: OLC: Cannot write to shop file!", BRF, LVL_BUILDER, TRUE);
     return;
  }

  /*. Search database for shops in this zone .*/
  for(i=zone *100; i <= top; i++)
  { rshop = real_shop(i);
    if (rshop != -1)
    { fprintf(shop_file, "#%d~\n", i);

      /*. Save products .*/
      for(j=0; S_PRODUCT(shop_index + rshop, j) != -1; j++)
        fprintf(shop_file, "%d\n", obj_index[S_PRODUCT(shop_index + rshop, j)].virtual);
      
      /*. Save rates .*/
      fprintf(shop_file, "-1\n%1.2f\n%1.2f\n", S_BUYPROFIT(shop_index + rshop), S_SELLPROFIT(shop_index + rshop));

      /*. Save buy types and namelists .*/
      j = -1;
      do
      { j++;
        fprintf(shop_file, "%d%s\n", 
          S_BUYTYPE(shop_index + rshop, j), 
          S_BUYWORD(shop_index + rshop, j) ? S_BUYWORD(shop_index + rshop, j) : ""
        );
      } while(S_BUYTYPE(shop_index + rshop, j) != -1);

      /*. Save messages'n'stuff .*/
      fprintf(shop_file,
	"%s~\n%s~\n%s~\n%s~\n%s~\n%s~\n%s~\n"
        "%d\n%d\n%d\n%d\n",
        /*. Added some small'n'silly defaults as sanity checks .*/
	S_NOITEM1(shop_index + rshop) ? S_NOITEM1(shop_index + rshop) : "%s Ke?!",
	S_NOITEM2(shop_index + rshop) ? S_NOITEM2(shop_index + rshop) : "%s Ke?!",
	S_NOBUY(shop_index + rshop) ? S_NOBUY(shop_index + rshop) : "%s Ke?!",
	S_NOCASH1(shop_index + rshop) ? S_NOCASH1(shop_index + rshop) : "%s Ke?!",
	S_NOCASH2(shop_index + rshop) ? S_NOCASH2(shop_index + rshop) : "%s Ke?!",
	S_BUY(shop_index + rshop) ? S_BUY(shop_index + rshop) : "%s Ke?! %d?",
	S_SELL(shop_index + rshop) ? S_SELL(shop_index + rshop) : "%s Ke?! %d?",
        S_BROKE_TEMPER(shop_index + rshop),
        S_BITVECTOR(shop_index + rshop),
        mob_index[S_KEEPER(shop_index + rshop)].virtual,
        S_NOTRADE(shop_index + rshop)
      );

      /*. Save rooms .*/
      j = -1;
      do
      { j++;
        fprintf(shop_file, "%d\n", S_ROOM(shop_index + rshop, j));
      } while(S_ROOM(shop_index + rshop, j) != -1);

      /*. Save open/closing times.*/
      fprintf(shop_file, "%d\n%d\n%d\n%d\n",
	S_OPEN1(shop_index + rshop),
	S_CLOSE1(shop_index + rshop),
	S_OPEN2(shop_index + rshop),
	S_CLOSE2(shop_index + rshop)
      );
    }
  }
  fprintf(shop_file, "$~\n");
  fclose(shop_file);
  olc_remove_from_save_list(zone_table[OLC_ZNUM(d)].number, OLC_SAVE_SHOP);
}


/**************************************************************************
 Menu functions 
 **************************************************************************/
  
void sedit_products_menu(struct descriptor_data *d)
{
  int i;

  get_char_cols(d->character);

  send_to_char("[H[J##     VNUM     Product\r\n", d->character);
  for(i=0; S_PRODUCT(OLC_SHOP(d), i) != -1; i++)
  { sprintf(buf, 
	"%2d - [%s%5d%s] - %s%s%s\r\n",  
        i,
	cyn, obj_index[S_PRODUCT(OLC_SHOP(d), i)].virtual, nrm,
        yel, obj_proto[S_PRODUCT(OLC_SHOP(d), i)].short_description, nrm
    );
    send_to_char(buf, d->character);
  }
  sprintf(buf, "\r\n"
	"%sA%s) Add a new product.\r\n"
	"%sD%s) Delete a product.\r\n"
        "%sQ%s) Quit\r\n"
        "Enter choice : ",
	grn, nrm, grn, nrm, grn, nrm);
  send_to_char(buf, d->character);
 
  OLC_MODE(d) = SEDIT_PRODUCTS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_compact_rooms_menu(struct descriptor_data *d)
{
  int i, count = 0;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for(i=0;  S_ROOM(OLC_SHOP(d), i) != -1; i++)
  { sprintf(buf, 
	"%2d - [%s%5d%s]  | ",  
        i, cyn, S_ROOM(OLC_SHOP(d), i), nrm
    );
    if (!(++count % 5))
      strcpy((buf + strlen(buf) -3), "\r\n");
    send_to_char(buf, d->character);
  }
  sprintf(buf, "\r\n"
	"%sA%s) Add a new room.\r\n"
	"%sD%s) Delete a room.\r\n"
        "%sL%s) Long display.\r\n"
        "%sQ%s) Quit\r\n"
        "Enter choice : ",
	grn, nrm, grn, nrm, grn, nrm, grn, nrm);
  send_to_char(buf, d->character);
 
  OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_rooms_menu(struct descriptor_data *d)
{
  int i;

  get_char_cols(d->character);

  send_to_char("[H[J##     VNUM     Room\r\n\r\n", d->character);
  for(i=0; S_ROOM(OLC_SHOP(d), i) != -1; i++)
  { sprintf(buf, 
	"%2d - [%s%5d%s] - %s%s%s\r\n",  
        i, cyn, S_ROOM(OLC_SHOP(d), i), nrm,
        yel, world[real_room(S_ROOM(OLC_SHOP(d), i))].name, nrm
    );
    send_to_char(buf, d->character);
  }
  sprintf(buf, "\r\n"
	"%sA%s) Add a new room.\r\n"
	"%sD%s) Delete a room.\r\n"
	"%sC%s) Compact Display.\r\n"
        "%sQ%s) Quit\r\n"
        "Enter choice : ",
	grn, nrm, grn, nrm, grn, nrm, grn, nrm);
  send_to_char(buf, d->character);
 
  OLC_MODE(d) = SEDIT_ROOMS_MENU;
}

/*-------------------------------------------------------------------*/

void sedit_namelist_menu(struct descriptor_data *d)
{
  int i;

  get_char_cols(d->character);

  send_to_char("[H[J##              Type   Namelist\r\n\r\n", d->character);
  for(i=0; S_BUYTYPE(OLC_SHOP(d), i) != -1; i++)
  { sprintf(buf, 
	"%2d - %s%15s%s - %s%s%s\r\n",  
        i, cyn, item_types[S_BUYTYPE(OLC_SHOP(d), i)], nrm,
        yel, S_BUYWORD(OLC_SHOP(d), i) ? S_BUYWORD(OLC_SHOP(d), i) : "<None>", nrm
    );
    send_to_char(buf, d->character);
  }
  sprintf(buf, "\r\n"
	"%sA%s) Add a new entry.\r\n"
	"%sD%s) Delete an entry.\r\n"
        "%sQ%s) Quit\r\n"
        "Enter choice : ",
	grn, nrm, grn, nrm, grn, nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = SEDIT_NAMELIST_MENU;
}
  
/*-------------------------------------------------------------------*/

void sedit_shop_flags_menu(struct descriptor_data *d)
{ int i, count =0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for(i = 0; i < NUM_SHOP_FLAGS; i++)
  { sprintf(buf,
	"%s%2d%s) %-20.20s   ",
        grn, i+1, nrm, shop_bits[i]
    );
    if (!(++count % 2))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  sprintbit(S_BITVECTOR(OLC_SHOP(d)), shop_bits, buf1);
  sprintf(buf, 
	"\r\nCurrent Shop Flags : %s%s%s\r\n"
	"Enter choice : ",
        cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
  OLC_MODE(d) = SEDIT_SHOP_FLAGS;
}

/*-------------------------------------------------------------------*/

void sedit_no_trade_menu(struct descriptor_data *d)
{ int i, count =0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for(i = 0; i < NUM_TRADERS; i++)
  { sprintf(buf,
	"%s%2d%s) %-20.20s   ",
        grn, i+1, nrm, trade_letters[i]
    );
    if (!(++count % 2))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  sprintbit(S_NOTRADE(OLC_SHOP(d)), trade_letters, buf1);
  sprintf(buf, 
	"\r\nCurrently won't trade with: %s%s%s\r\n"
	"Enter choice : ",
        cyn, buf1, nrm
  );
  send_to_char(buf, d->character);
  OLC_MODE(d) = SEDIT_NOTRADE;
}

/*-------------------------------------------------------------------*/

void sedit_types_menu(struct descriptor_data *d)
{ 
  int i, count = 0;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for(i=0;  i < NUM_ITEM_TYPES; i++)
  { sprintf(buf, 
	"%s%2d%s) %s%-20s%s  ",  
        grn, i, nrm,
	cyn, item_types[i], nrm
    );
    if(!(++count % 3))
      strcat(buf, "\r\n");
    send_to_char(buf, d->character);
  }
  sprintf(buf, "%sEnter choice : ", nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = SEDIT_TYPE_MENU;
}
    

/*-------------------------------------------------------------------*/
/*. Display main menu .*/

void sedit_disp_menu(struct descriptor_data *d)
{ struct shop_data *shop;

  shop = OLC_SHOP(d);
  get_char_cols(d->character);

  sprintbit(S_NOTRADE(shop), trade_letters, buf1); 
  sprintbit(S_BITVECTOR(shop), shop_bits, buf2); 
  sprintf(buf, "[H[J"
	"-- Shop Number : [%s%d%s]\r\n"
	"%s0%s) Keeper      : [%s%d%s] %s%s\r\n"
        "%s1%s) Open 1      : %s%4d%s          %s2%s) Close 1     : %s%4d\r\n"
        "%s3%s) Open 2      : %s%4d%s          %s4%s) Close 2     : %s%4d\r\n"
        "%s5%s) Sell rate   : %s%1.2f%s          %s6%s) Buy rate    : %s%1.2f\r\n"
        "%s7%s) Keeper no item : %s%s\r\n"
        "%s8%s) Player no item : %s%s\r\n"
        "%s9%s) Keeper no cash : %s%s\r\n"
        "%sA%s) Player no cash : %s%s\r\n"
        "%sB%s) Keeper no buy  : %s%s\r\n"
        "%sC%s) Buy sucess     : %s%s\r\n"
        "%sD%s) Sell sucess    : %s%s\r\n"
        "%sE%s) No Trade With  : %s%s\r\n"
        "%sF%s) Shop flags     : %s%s\r\n"
        "%sR%s) Rooms Menu\r\n"
        "%sP%s) Products Menu\r\n"
        "%sT%s) Accept Types Menu\r\n"
        "%sQ%s) Quit\r\n"
        "Enter Choice : ",

	cyn, OLC_NUM(d), nrm,
	grn, nrm, cyn, S_KEEPER(shop) == -1 ?
                       -1 : mob_index[S_KEEPER(shop)].virtual, nrm,
                  yel, S_KEEPER(shop) == -1 ?
                       "None" : mob_proto[S_KEEPER(shop)].player.short_descr,
	grn, nrm, cyn, S_OPEN1(shop), nrm,
	grn, nrm, cyn, S_CLOSE1(shop),
	grn, nrm, cyn, S_OPEN2(shop), nrm,
	grn, nrm, cyn, S_CLOSE2(shop),
	grn, nrm, cyn, S_BUYPROFIT(shop), nrm,
	grn, nrm, cyn, S_SELLPROFIT(shop),
	grn, nrm, yel, S_NOITEM1(shop),
	grn, nrm, yel, S_NOITEM2(shop),
	grn, nrm, yel, S_NOCASH1(shop),
	grn, nrm, yel, S_NOCASH2(shop),
	grn, nrm, yel, S_NOBUY(shop),
	grn, nrm, yel, S_BUY(shop),
	grn, nrm, yel, S_SELL(shop),
        grn, nrm, cyn, buf1,
        grn, nrm, cyn, buf2,
	grn, nrm, grn, nrm, grn, nrm, grn, nrm
  );
  send_to_char(buf, d->character);

  OLC_MODE(d) = SEDIT_MAIN_MENU;
}

/**************************************************************************
  The GARGANTUAN event handler
 **************************************************************************/

void sedit_parse(struct descriptor_data * d, char *arg)
{ int i;

  if (OLC_MODE(d) > SEDIT_NUMERICAL_RESPONSE) {
    if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && !isdigit(arg[1])))) {
      write_to_output("Field must be numerical, try again : ", d);
      return;
    }
  }

  switch (OLC_MODE(d)) {
/*-------------------------------------------------------------------*/
  case SEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      sedit_save_internally(d);
      sprintf(buf, "OLC: %s edits shop %d", GET_NAME(d->character),
	      OLC_NUM(d));
      mudlog(buf, CMP, LVL_BUILDER, TRUE);
      cleanup_olc(d, CLEANUP_ALL);
      break;
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      write_to_output("Invalid choice!\r\n", d);
      write_to_output("Do you wish to save this shop? ", d);
      break;
    }
    return;
  case SEDIT_MAIN_MENU:
    i = 0;
    switch (*arg) 
    { case 'q':
      case 'Q':
        if (OLC_VAL(d)) /*. Anything been changed? .*/
        { send_to_char("Do you wish to save the changes to the shop? (y/n) : ", d->character);
          OLC_MODE(d) = SEDIT_CONFIRM_SAVESTRING;
        } else
          cleanup_olc(d, CLEANUP_ALL);
        return;
      case '0':
        OLC_MODE(d) = SEDIT_KEEPER;
        send_to_char("Enter virtual number of shop keeper : ", d->character);
        return;
      case '1':
        OLC_MODE(d) = SEDIT_OPEN1;
        i++;
        break;
      case '2':
        OLC_MODE(d) = SEDIT_CLOSE1;
        i++;
        break;
      case '3':
        OLC_MODE(d) = SEDIT_OPEN2;
        i++;
        break;
      case '4':
        OLC_MODE(d) = SEDIT_CLOSE2;
        i++;
        break;
      case '5':
        OLC_MODE(d) = SEDIT_BUY_PROFIT;
        i++;
        break;
      case '6':
        OLC_MODE(d) = SEDIT_SELL_PROFIT;
        i++;
        break;
      case '7':
        OLC_MODE(d) = SEDIT_NOITEM1;
        i--;
        break;
      case '8':
        OLC_MODE(d) = SEDIT_NOITEM2;
        i--;
        break;
      case '9':
        OLC_MODE(d) = SEDIT_NOCASH1;
        i--;
        break;
      case 'a':
      case 'A':
        OLC_MODE(d) = SEDIT_NOCASH2;
        i--;
        break;
      case 'b':
      case 'B':
        OLC_MODE(d) = SEDIT_NOBUY;
        i--;
        break;
      case 'c':
      case 'C':
        OLC_MODE(d) = SEDIT_BUY;
        i--;
        break;
      case 'd':
      case 'D':
        OLC_MODE(d) = SEDIT_SELL;
        i--;
        break;
      case 'e':
      case 'E':
        sedit_no_trade_menu(d);
        return;
      case 'f':
      case 'F':
        sedit_shop_flags_menu(d);
        return;
      case 'r':
      case 'R':
        sedit_rooms_menu(d);
        return;
      case 'p':
      case 'P':
        sedit_products_menu(d);
        return;
      case 't':
      case 'T':
        sedit_namelist_menu(d);
        return;
      default:
        sedit_disp_menu(d);
	return;
    }

    if (i==1)
    {  send_to_char("\r\nEnter new value : ", d->character);
       return;
    }
    if (i==-1)
    {  send_to_char("\r\nEnter new text :\r\n| ", d->character);
       return;
    }
    break; 
/*-------------------------------------------------------------------*/
  case SEDIT_NAMELIST_MENU:
    switch (*arg) 
    { case 'a':
      case 'A':
        sedit_types_menu(d);
        return;
      case 'd':
      case 'D':
        send_to_char("\r\nDelete which entry? : ", d->character);
        OLC_MODE(d) = SEDIT_DELETE_TYPE;
        return;
      case 'q':
      case 'Q':
        break;
    }
    break;
/*-------------------------------------------------------------------*/
  case SEDIT_PRODUCTS_MENU:
    switch (*arg) 
    { case 'a':
      case 'A':
        send_to_char("\r\nEnter new product virtual number : ", d->character);
        OLC_MODE(d) = SEDIT_NEW_PRODUCT;
        return;
      case 'd':
      case 'D':
        send_to_char("\r\nDelete which product? : ", d->character);
        OLC_MODE(d) = SEDIT_DELETE_PRODUCT;
        return;
      case 'q':
      case 'Q':
        break;
    }
    break;
/*-------------------------------------------------------------------*/
  case SEDIT_ROOMS_MENU:
    switch (*arg) 
    { case 'a':
      case 'A':
        send_to_char("\r\nEnter new room virtual number : ", d->character);
        OLC_MODE(d) = SEDIT_NEW_ROOM;
        return;
      case 'c':
      case 'C':
        sedit_compact_rooms_menu(d);
        return;
      case 'l':
      case 'L':
        sedit_rooms_menu(d);
        return;
      case 'd':
      case 'D':
        send_to_char("\r\nDelete which room? : ", d->character);
        OLC_MODE(d) = SEDIT_DELETE_ROOM;
        return;
      case 'q':
      case 'Q':
        break;
    }
    break;
/*-------------------------------------------------------------------*/
  /*. String edits .*/
  case SEDIT_NOITEM1:
    sedit_modify_string(&S_NOITEM1(OLC_SHOP(d)), arg);
    break;
  case SEDIT_NOITEM2:
    sedit_modify_string(&S_NOITEM2(OLC_SHOP(d)), arg);
    break;
  case SEDIT_NOCASH1:
    sedit_modify_string(&S_NOCASH1(OLC_SHOP(d)), arg);
    break;
  case SEDIT_NOCASH2:
    sedit_modify_string(&S_NOCASH2(OLC_SHOP(d)), arg);
    break;
  case SEDIT_NOBUY:
    sedit_modify_string(&S_NOBUY(OLC_SHOP(d)), arg);
    break;
  case SEDIT_BUY:
    sedit_modify_string(&S_BUY(OLC_SHOP(d)), arg);
    break;
  case SEDIT_SELL:
    sedit_modify_string(&S_SELL(OLC_SHOP(d)), arg);
    break;
  case SEDIT_NAMELIST:
    { struct shop_buy_data new_entry;
      BUY_TYPE(new_entry) = OLC_VAL(d);
      if(*arg)
        BUY_WORD(new_entry) = str_dup(arg); 
      else
        BUY_WORD(new_entry) = NULL;
      sedit_add_to_type_list(&(S_NAMELISTS(OLC_SHOP(d))), &new_entry);
    }
    sedit_namelist_menu(d);
    return;

/*-------------------------------------------------------------------*/
  /*. Numerical responses .*/

  case SEDIT_KEEPER:
    i = atoi(arg);
    if (i != -1)
    { i = real_mobile(i);
      if (i < 0)
      { send_to_char("That mobile does not exist, try again : ", d->character);
        return;
      }
    }
    S_KEEPER(OLC_SHOP(d)) = i;
    if (i == -1)
      break;
    /*. Fiddle with special procs .*/
    S_FUNC(OLC_SHOP(d)) = mob_index[i].func;
    mob_index[i].func = shop_keeper;
    break;
  case SEDIT_OPEN1:
    S_OPEN1(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
    break;
  case SEDIT_OPEN2:
    S_OPEN2(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
    break;
  case SEDIT_CLOSE1:
    S_CLOSE1(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
    break;
  case SEDIT_CLOSE2:
    S_CLOSE2(OLC_SHOP(d)) = MAX(0, MIN(28, atoi(arg)));
    break;
  case SEDIT_BUY_PROFIT:
    sscanf(arg, "%f", &S_BUYPROFIT(OLC_SHOP(d)));
    break;
  case SEDIT_SELL_PROFIT:
    sscanf(arg, "%f", &S_SELLPROFIT(OLC_SHOP(d)));
    break;
  case SEDIT_TYPE_MENU:
    OLC_VAL(d) = MAX(0, MIN(NUM_ITEM_TYPES - 1, atoi(arg)));
    send_to_char("Enter namelist (return for none) :-\r\n| ", d->character);
    OLC_MODE(d) = SEDIT_NAMELIST;
    return;
  case SEDIT_DELETE_TYPE:
    sedit_remove_from_type_list(&(S_NAMELISTS(OLC_SHOP(d))), atoi(arg));
    sedit_namelist_menu(d);
    return;
  case SEDIT_NEW_PRODUCT:
    i = atoi(arg);
    if (i != -1)
    { i = real_object(i);
      if (i == -1)
      { send_to_char("That object does not exist, try again : ", d->character);
        return;
      }
    } 
    if (i > 0)
      sedit_add_to_int_list(&(S_PRODUCTS(OLC_SHOP(d))), i);
    sedit_products_menu(d);
    return;
  case SEDIT_DELETE_PRODUCT:
    sedit_remove_from_int_list(&(S_PRODUCTS(OLC_SHOP(d))), atoi(arg));
    sedit_products_menu(d);
    return;
  case SEDIT_NEW_ROOM:
    i = atoi(arg);
    if (i != -1)
    { i = real_room(i);
      if (i < 0)
      { send_to_char("That room does not exist, try again : ", d->character);
        return;
      }
    }
    if (i >= 0)
      sedit_add_to_int_list(&(S_ROOMS(OLC_SHOP(d))), atoi(arg));
    sedit_rooms_menu(d);
    return;
  case SEDIT_DELETE_ROOM:
    sedit_remove_from_int_list(&(S_ROOMS(OLC_SHOP(d))), atoi(arg));
    sedit_rooms_menu(d);
    return;
  case SEDIT_SHOP_FLAGS:
    i = MAX(0, MIN(NUM_SHOP_FLAGS, atoi(arg)));
    if (i>0)
    { /*. Toggle bit .*/
      i = 1 << (i-1);
      if(IS_SET(S_BITVECTOR(OLC_SHOP(d)), i))
        REMOVE_BIT(S_BITVECTOR(OLC_SHOP(d)), i);
      else
        SET_BIT(S_BITVECTOR(OLC_SHOP(d)), i);
      sedit_shop_flags_menu(d);
      return;
    }
    break;
  case SEDIT_NOTRADE:
    i = MAX(0, MIN(NUM_TRADERS, atoi(arg)));
    if (i>0)
    { /*. Toggle bit .*/
      i = 1 << (i-1);
      if(IS_SET(S_NOTRADE(OLC_SHOP(d)), i))
        REMOVE_BIT(S_NOTRADE(OLC_SHOP(d)), i);
      else
        SET_BIT(S_NOTRADE(OLC_SHOP(d)), i);
      sedit_no_trade_menu(d);
      return;
    }
    break;

/*-------------------------------------------------------------------*/
  default:
    /*. We should never get here .*/
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: sedit_parse(): Reached default case!",BRF,LVL_BUILDER,TRUE);
    break;
  }
/*-------------------------------------------------------------------*/
/*. END OF CASE 
    If we get here, we have probably changed something, and now want to
    return to main menu.  Use OLC_VAL as a 'has changed' flag .*/

  OLC_VAL(d) = 1;
  sedit_disp_menu(d);
}

