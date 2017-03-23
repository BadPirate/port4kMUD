#include "conf.h" 
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"

#include "auction.h"

extern struct descriptor_data *descriptor_list;
extern struct room_data *world;

/*
 * auction_output : takes two strings and dispenses them to everyone connected
 *		based on if they have color on or not.  Note that the buf's are
 *		commonly used *color and *black so I allocate my own buffer.
 */
void auction_output(char *color, char *black)
{
  char buffer[MAX_STRING_LENGTH];
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
    if (!d->connected && d->character &&
       !PLR_FLAGGED(d->character, PLR_WRITING) &&
       !PRF_FLAGGED(d->character, PRF_NOAUCT) &&
       !ROOM_FLAGGED(d->character->in_room, ROOM_SOUNDPROOF)) {
      sprintf(buffer, "%s[%sAUCTION:%s %s%s]%s\r\n",
	CCMAG(d->character,C_NRM),CCCYN(d->character,C_NRM),
	CCNRM(d->character,C_NRM),
	(COLOR_LEV(d->character) > C_NRM) ? color : black,
	CCMAG(d->character,C_NRM),CCNRM(d->character,C_NRM));
      send_to_char(buffer, d->character);
    }
}

void auction_update(void)
{
  sh_int auction_room;
  auction_room = real_room(175);

  if (auction.ticks == AUC_NONE) /* No auction */
    return;  

  /* Seller left! */
  if (!get_ch_by_id(auction.seller)) {
    if (auction.obj)
      extract_obj(auction.obj);
    auction_reset();
    return;
  }

  /* If there is an auction but it's not sold yet */
  if (auction.ticks >= AUC_BID && auction.ticks <= AUC_SOLD) {
    /* If there is a bidder and it's not sold yet */
    if (get_ch_by_id(auction.bidder) && (auction.ticks < AUC_SOLD)) {
      /* Non colored message */
      sprintf(buf, "%s is going %s%s%s to %s for %ld coin%s.",
          auction.obj->short_description,
          auction.ticks == AUC_BID ? "once" : "",
          auction.ticks == AUC_ONCE ? "twice" : "",
          auction.ticks == AUC_TWICE ? "for the last call" : "",
          get_ch_by_id(auction.bidder)->player.name,
          auction.bid, auction.bid != 1 ? "s" : " ");
      /* Colored message */
      sprintf(buf2, "\x1B[1;37m%s\x1B[35m is going \x1B[1;37m%s%s%s\x1B[35m to \x1B[1;37m%s\x1B[35m for \x1B[1;37m%ld\x1B[35m coin%s.",
          auction.obj->short_description,
          auction.ticks == AUC_BID ? "once" : "",
          auction.ticks == AUC_ONCE ? "twice" : "",
          auction.ticks == AUC_TWICE ? "for the last call" : "",
          get_ch_by_id(auction.bidder)->player.name,
          auction.bid, auction.bid != 1 ? "s" : " ");
      /* send the output */
      auction_output(buf2, buf);
      /* Increment timer */
      auction.ticks++;
      return;
    }

    /* If there is no bidder and we ARE in the sold state */
    if (!get_ch_by_id(auction.bidder) && (auction.ticks == AUC_SOLD)) {
      /* Colored message */
      sprintf(buf2, "\x1B[1;37m%s\x1B[35m is \x1B[1;37mSOLD\x1B[35m to \x1B[1;37mno one\x1B[35m for \x1B[1;37m%ld\x1B[35m coin%s.",
          auction.obj->short_description,
          auction.bid, auction.bid != 1 ? "s" : " ");
      /* No color message */
      sprintf(buf, "%s is SOLD to no one for %ld coin%s.",
          auction.obj->short_description,
          auction.bid,
          auction.bid != 1 ? "s" : " ");
      /* Send the output away */
      auction_output(buf2, buf);
      /* Give the poor fellow his unsold goods back */
      obj_to_char(auction.obj, get_ch_by_id(auction.seller));
      /* Reset the auction for next time */
      auction_reset();
      return;
    }

    /* If there is no bidder and we are not in the sold state */
    if (!get_ch_by_id(auction.bidder) && (auction.ticks < AUC_SOLD)) {
      /* Colored output message */
      sprintf(buf2, "\x1B[1;37m%s\x1B[35m is going \x1B[1;37m%s%s%s\x1B[35m to \x1B[1;37mno one\x1B[35m for \x1B[1;37m%ld\x1B[35m coin%s.",
          auction.obj->short_description,
          auction.ticks == AUC_BID ? "once" : "",
          auction.ticks == AUC_ONCE ? "twice" : "",
          auction.ticks == AUC_TWICE ? "for the last call" : "",
          auction.bid, auction.bid != 1 ? "s" : "");
      /* No color output message */
      sprintf(buf, "%s is going %s%s%s to no one for %ld coin%s.",
          auction.obj->short_description,
          auction.ticks == AUC_BID ? "once" : "",
          auction.ticks == AUC_ONCE ? "twice" : "",
          auction.ticks == AUC_TWICE ? "for the last call" : "",
          auction.bid, auction.bid != 1 ? "s" : "");
      /* Send output away */
      auction_output(buf2, buf);
      /* Increment timer */
      auction.ticks++;
      return;
    }

    /* Sold */
    if (get_ch_by_id(auction.bidder) && (auction.ticks >= AUC_SOLD)) {
      /* Get pointers to the bidder and seller */
      struct char_data *seller = get_ch_by_id(auction.seller);
      struct char_data *bidder = get_ch_by_id(auction.bidder);

      /* Colored output */
      sprintf(buf2, "\x1B[1;37m%s\x1B[35m is \x1B[1;37mSOLD\x1B[35m to \x1B[1;37m%s\x1B[35m for \x1B[1;37m%ld\x1B[35m coin%s.",
          auction.obj->short_description ? auction.obj->short_description : "something",
          bidder->player.name ? bidder->player.name : "someone",
          auction.bid, auction.bid != 1 ? "s" : "");
      /* Non color output */
      sprintf(buf, "%s is SOLD to %s for %ld coin%s.",
          auction.obj->short_description ? auction.obj->short_description : "something",
          bidder->player.name ? bidder->player.name : "someone",
          auction.bid, auction.bid != 1 ? "s" : "");
      /* Send the output */
      auction_output(buf2, buf);
  
      /* If the seller is still around we give him the money */
      if (seller) {
	GET_GOLD(seller) += auction.bid;
        act("Congrats! You have sold $p!", FALSE, seller, auction.obj, 0, TO_CHAR);
      }
      /* If the bidder is here he gets the object */
      if (bidder) {
	obj_to_char(auction.obj, bidder);
	act("Congrats! You now have $p!", FALSE, bidder, auction.obj, 0, TO_CHAR);
      }
      
      /* put in to add realism and avoid cheesing */
      char_from_room(bidder);
      char_from_room(seller);
      char_to_room(bidder, auction_room);
      char_to_room(seller, auction_room);
      look_at_room(seller,seller->in_room);
      look_at_room(bidder,bidder->in_room);
      send_to_char("You complete the transaction.\r\n",bidder);
      send_to_char("You complete the transaction.\r\n",seller);

      /* Restore the status of the auction */
      auction_reset();
      return;
    }
  }
  return;
}

/*
 * do_bid : user interface to place a bid.
 */

ACMD(do_aucid)
{
  int i;
  int found;
  int ctoid;
  int slfid;

  struct time_info_data age(struct char_data * ch);
  extern char *spells[];
  extern char *item_types[];
  extern char *extra_bits[];
  extern char *apply_types[];
  extern char *affected_bits[];

  if (auction.ticks == AUC_NONE) {
    send_to_char("Nothing is up for sale.\r\n", ch);
    return;
  }

  ctoid=2301;
  slfid=0;
  if(ch==get_ch_by_id(auction.seller)) {
    ctoid*=6;
    slfid=1;
  }

  if (auction.obj) {
    if (GET_GOLD(ch) < ctoid)
    {
      sprintf(buf,"It will cost you %d gold to identify %s.\r\n",ctoid,
              slfid?"your own auction":"that auction");
      send_to_char(buf,ch);
      return;
    } else {
      sprintf(buf,"It costed you %d gold to identify %s:\r\n",ctoid,
              slfid?"your own auction":"that auction");
      send_to_char(buf,ch);
    }
    GET_GOLD(ch) -= ctoid;
    send_to_char("You feel informed:\r\n", ch);
    sprintf(buf, "Object '%s', Item type: ", auction.obj->short_description);
    sprinttype(GET_OBJ_TYPE(auction.obj), item_types, buf2);
    strcat(buf, buf2);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    if (auction.obj->obj_flags.bitvector) {
      send_to_char("Item will give you following abilities:  ", ch);
      sprintbit(auction.obj->obj_flags.bitvector, affected_bits, buf);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }
    send_to_char("Item is: ", ch);
    sprintbit(GET_OBJ_EXTRA(auction.obj), extra_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    sprintf(buf, "Weight: %d, Value: %d, Rent: %d\r\n",
            GET_OBJ_WEIGHT(auction.obj), GET_OBJ_COST(auction.obj), GET_OBJ_RENT(auction.obj));
    send_to_char(buf, ch);

    switch (GET_OBJ_TYPE(auction.obj)) {
    case ITEM_SCROLL:
    case ITEM_POTION:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(auction.obj)]);

      if (GET_OBJ_VAL(auction.obj, 1) >= 1)
        sprintf(buf, "%s %s", buf, spells[GET_OBJ_VAL(auction.obj, 1)]);
      if (GET_OBJ_VAL(auction.obj, 2) >= 1)
        sprintf(buf, "%s %s", buf, spells[GET_OBJ_VAL(auction.obj, 2)]);
      if (GET_OBJ_VAL(auction.obj, 3) >= 1)
        sprintf(buf, "%s %s", buf, spells[GET_OBJ_VAL(auction.obj, 3)]);
      sprintf(buf, "%s\r\n", buf);
      send_to_char(buf, ch);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(auction.obj)]);
      sprintf(buf, "%s %s\r\n", buf, spells[GET_OBJ_VAL(auction.obj, 3)]);
      sprintf(buf, "%sIt has %d maximum charge%s and %d remaining.\r\n", buf,
              GET_OBJ_VAL(auction.obj, 1), GET_OBJ_VAL(auction.obj, 1) == 1 ? "" : "s",
              GET_OBJ_VAL(auction.obj, 2));
      send_to_char(buf, ch);
      break;
    case ITEM_WEAPON:
      sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(auction.obj, 1),
              GET_OBJ_VAL(auction.obj, 2));
      sprintf(buf, "%s for an average per-round damage of %.1f.\r\n", buf,
              (((GET_OBJ_VAL(auction.obj, 2) + 1) / 2.0) * GET_OBJ_VAL(auction.obj, 1)));
      send_to_char(buf, ch);
      break;
    case ITEM_ARMOR:
      sprintf(buf, "AC-apply is %d\r\n", GET_OBJ_VAL(auction.obj, 0));
      send_to_char(buf, ch);
      break;
    }
    found = FALSE;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((auction.obj->affected[i].location != APPLY_NONE) &&
          (auction.obj->affected[i].modifier != 0)) {
        if (!found) {
          send_to_char("Can affect you as :\r\n", ch);
          found = TRUE;
        }
        sprinttype(auction.obj->affected[i].location, apply_types, buf2);
        sprintf(buf, "   Affects: %s By %d\r\n", buf2, auction.obj->affected[i].modifier);
        send_to_char(buf, ch);
      }
    }
  }
}
ACMD(do_bid)
{
  long bid;

  /* NPC's can not bid or auction if charmed */
  if (ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    return;

  /* There isn't an auction */
  if (auction.ticks == AUC_NONE) {
    send_to_char("Nothing is up for sale.\r\n", ch);
    return;
  }

  if ( IS_SET(ROOM_FLAGS(ch->in_room), ROOM_NORECALL) &&
       GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char("The room your in prevents you from auctioning.\r\n",ch);
    return;
  }

  one_argument(argument, buf);
  bid = atoi(buf);

  /* They didn't type anything else */
  if (!*buf) {
    sprintf(buf2, "Current bid: %ld coin%s\r\n", auction.bid,
        auction.bid != 1 ? "s." : ".");
    send_to_char(buf2, ch);
  /* The current bidder is this person */
  } else if (ch == get_ch_by_id(auction.bidder))
    send_to_char("You're trying to outbid yourself.\r\n", ch);
  /* The seller is the person who tried to bid */
  else if (ch == get_ch_by_id(auction.seller))
    send_to_char("You can't bid on your own item.\r\n", ch);
  /* Tried to auction below the minimum */
  else if ((bid < auction.bid) && !get_ch_by_id(auction.bidder)) {
    sprintf(buf2, "The minimum is currently %ld coins.\r\n", auction.bid);
    send_to_char(buf2, ch);
  /* Tried to bid below the minimum where there is a bid, 5% increases */
  } else if ((bid < (auction.bid * 1.05) && get_ch_by_id(auction.bidder)) || bid == 0) {
    sprintf(buf2, "Try bidding at least 5%% over the current bid of %ld. (%.0f coins).\r\n",
        auction.bid, auction.bid * 1.05 + 1);
    send_to_char(buf2, ch);
  /* Not enough gold on hand! */
  } else if (GET_GOLD(ch) < bid) {
    sprintf(buf2, "You have only %d coins on hand.\r\n", GET_GOLD(ch));
    send_to_char(buf2, ch);
  /* it's an ok bid */
  } else {
    /* Give last bidder money back if he's around! */
    if (get_ch_by_id(auction.bidder))
      GET_GOLD(get_ch_by_id(auction.bidder)) += auction.bid;
    /* This is the bid value */
    auction.bid = bid;
    /* The bidder is this guy */
    auction.bidder = GET_IDNUM(ch);
    /* This resets the auction to first chance bid */
    auction.ticks = AUC_BID;
    /* Get money from new bidder. */
    GET_GOLD(get_ch_by_id(auction.bidder)) -= auction.bid;
    /* Prepare colored message */
    sprintf(buf2, "\x1B[1;37m%s\x1B[35m bids \x1B[1;37m%ld\x1B[35m coin%s on \x1B[1;37m%s\x1B[35m.",
	get_ch_by_id(auction.bidder)->player.name,
	auction.bid,
	auction.bid!=1 ? "s" :"",
	auction.obj->short_description);
    /* Prepare non-colored message */
    sprintf(buf, "%s bids %ld coin%s on %s.",
	get_ch_by_id(auction.bidder)->player.name,
	auction.bid,
	auction.bid!=1 ? "s" :"",
	auction.obj->short_description);
    /* Send the output away */
    auction_output(buf2, buf);
  }
}

/*
 * do_auction : user interface for placing an item up for sale
 */
ACMD(do_auction)
{
  struct obj_data *obj;
  struct char_data *seller;

  /* NPC's can not bid or auction if charmed */
  if (ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if ( IS_SET(ROOM_FLAGS(ch->in_room), ROOM_NORECALL) &&
       GET_LEVEL(ch) < LVL_IMMORT)
  {
    send_to_char("You feel powerless to auction in this room.\r\n",ch);
    return;
  }
  two_arguments(argument, buf1, buf2);

  seller = get_ch_by_id(auction.seller);

  /* There is nothing they typed */
  if (!*buf1)
    send_to_char("Auction what for what minimum?\r\n", ch);
  /* Hrm...logic error? */
  else if (auction.ticks != AUC_NONE) {
    /* If seller is no longer present, auction continues with no seller */
    if (seller) {
      sprintf(buf2, "%s is currently auctioning %s for %ld coins.\r\n",
        get_ch_by_id(auction.seller)->player.name,
        auction.obj->short_description, auction.bid);
      send_to_char(buf2, ch);
    } else {
      sprintf(buf2, "No one is currently auctioning %s for %ld coins.\r\n",
         auction.obj->short_description, auction.bid);
      send_to_char(buf2, ch);
    }
  /* Person doesn't have that item */
  } else if ((obj = get_obj_in_list_vis(ch, buf1, ch->carrying)) == NULL)
    send_to_char("You don't seem to have that to sell.\r\n", ch);
  /* Can not auction corpses because they may decompose */
  else if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && (GET_OBJ_VAL(obj, 3)))
     send_to_char("You can not auction corpses.\n\r", ch);
  /* It's valid */
  else {
    /* First bid attempt */
    auction.ticks = AUC_BID;
    /* This guy is selling it */
    auction.seller = GET_IDNUM(ch);
    /* Can not make the minimum less than 0 --KR */
    auction.bid = (atoi(buf2) > 0 ? atoi(buf2) : 1);
    /* Pointer to object */
    auction.obj = obj;
    /* Get the object from the character, so they cannot drop it! */
    obj_from_char(auction.obj);
    /* Prepare color message for those with it */
    sprintf(buf2, "\x1B[1;37m%s\x1B[35m puts \x1B[1;37m%s\x1B[35m up for sale, minimum bid \x1B[1;37m%ld\x1B[35m coin%s",
	get_ch_by_id(auction.seller)->player.name,
	auction.obj->short_description,
	auction.bid, auction.bid != 1 ? "s." : ".");
    /* Make a message sans-color for those whole have it off */
    sprintf(buf, "%s puts %s up for sale, minimum bid %ld coin%s",
	get_ch_by_id(auction.seller)->player.name,
	auction.obj->short_description,
	auction.bid, auction.bid != 1 ? "s." : ".");
    /* send out the messages */
    auction_output(buf2, buf);
  }
}

/*
 * auction_reset : returns the auction structure to a non-bidding state
 */
void auction_reset(void)
{
  auction.bidder = -1;
  auction.seller = -1;
  auction.obj = NULL;
  auction.ticks = AUC_NONE;
  auction.bid = 0;
}

/*
 * get_ch_by_id : given an ID number, searches every descriptor for a
 *		character with that number and returns a pointer to it.
 */
struct char_data *get_ch_by_id(long idnum)
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
    if (d && d->character && GET_IDNUM(d->character) == idnum)
      return (d->character);

  return NULL;
}
