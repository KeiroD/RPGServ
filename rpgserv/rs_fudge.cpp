/*
 * RPGServ roll+ functions
 *
 */

#include "module.h"

class CommandRSFudge : public Command
{
 private:

	int ran(int k) {
		double x = RAND_MAX + 1.0;
		int y;
		y = 1 + rand() * (k / x);
		return ( y );
	}

	int roll(int many, int sides) {
		int i,q=0;
		int dicemax = Config->GetModule("rpgserv")->Get<int>("dicemax","15");
		int sidemax = Config->GetModule("rpgserv")->Get<int>("sidemax","100");
		if(many>dicemax)
			many=dicemax;
		if(sides>sidemax)
			sides=sidemax;
		for (i=0;i<many;i++) {
			q += ran(sides);
		}
		return ( q );
	}

 public:
	CommandRSFudge(Module *creator) : Command(creator, "rpgserv/fudge", 1, 3)
	{
		this->SetDesc(_("Rolls a specifed number of fudge dice."));
		this->SetSyntax(_("\002\037<dnumber of ice>\037 \037#channel\037 \037[message]\037\002"));
		this->AllowUnregistered(true);
	}
	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		const Anope::string &dice = params[0];
		const Anope::string &chan = params.size() > 1 ? params[1] : "";
		const Anope::string &message = params.size() > 2 ? params[2] : "";
		Anope::string target;
		int dicemax = Config->GetModule("rpgserv")->Get<int>("dicemax","15");
		int X;
		int i;
      int myroll;
		int total = 0;
		User *u2 = NULL;
		srand(time(NULL));
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
		User *u = source.GetUser();
      if (dice.empty() || !dice.is_pos_number_only() )
      {
         source.Reply(_("You must specify a positive number of dice to roll."));
         return;
      }
		X = convertTo<int>(dice);
      if (X > dicemax)
      {
         source.Reply(_("You have a maximum of %d dice."),dicemax);
         return;
      }

		if(!chan.empty()) 
		{
			if (chan[0] == '#')
			{
				if (!IRCD->IsChannelValid(chan))
					source.Reply(CHAN_X_INVALID, chan.c_str());
				else 
				{
					ChannelInfo *ci = ChannelInfo::Find(chan);
					if (!ci && u)
					{
						for (channel_map::const_iterator cit = ChannelList.begin(), cit_end = ChannelList.end(); cit != cit_end; ++cit)
						{
							Channel *c = cit->second;
							if (!Anope::Match(c->name, chan, false, true))
								continue;
							target = chan.c_str();
						}
						if (target.empty()) {
							source.Reply(CHAN_X_NOT_IN_USE, chan.c_str());
							return;
						}
					}
					else
					{
						target = ci->name.c_str(); 
					}
				}
			}
			else
			{
				u2 = User::Find(chan, true);
				if (!u2) 
				{
					source.Reply(NICK_X_NOT_IN_USE, chan.c_str());
					return;
				}
				else 
				{
					target = u2->nick.c_str();
				}
			}
		}

//do the work
      for (i=0; i<X; i++)
      {
        myroll = roll(1,3)-2;
        total = total + myroll;
      }

		if (!target.empty())
		{
			if (!u2)
			{
				IRCD->SendPrivmsg(bi, target.c_str(), "%s Fudged %s dice for %d: %s", u->nick.c_str(), dice.c_str(), total, message.c_str());
			}
			else
			{
				u2->SendMessage(bi, _("%s Fudged %s dice for %d: %s"),u->nick.c_str(), dice.c_str(), total, message.c_str());
			}
			source.Reply("Fudged %s dice for %d: %s",dice.c_str(), total, message.c_str());

		}
		else
		{
			u->SendMessage(bi, _("%s Fudged %s dice for %d: %s"), u->nick.c_str(), dice.c_str(), total, message.c_str());
		}
		return;
	}
	bool OnHelp(CommandSource &source, const Anope::string &subcommand) anope_override
	{
		int dicemax = Config->GetModule("rpgserv")->Get<int>("dicemax","30");
		int sidemax = Config->GetModule("rpgserv")->Get<int>("sidemax","100");
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply("No help yet");
		return true;
	}
};

class RSFudge : public Module
{
	CommandRSFudge commandrsfudge;

 public:
	RSFudge(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA),
		commandrsfudge(this)
	{
		this->SetAuthor("Azander");
		this->SetVersion("2.0");
	}
};


MODULE_INIT(RSFudge)
