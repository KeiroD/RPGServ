/*
 * RPGServ say/act function
 */

#include "module.h"

class CommandRSAct : public Command
{
 public:
	CommandRSAct(Module *creator) : Command(creator, "rpgserv/act", 1, 2)
	{
		this->SetDesc(_("Does something"));
		this->SetSyntax(_("\002\037where\037 \037<what was done>\037\002"));
		this->AllowUnregistered(true);
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) anope_override
	{
		const Anope::string &chan = params[0];
		const Anope::string &message = params.size() > 1 ? params[1] : "";
		//Get Service info for messaging
		BotInfo *bi = BotInfo::Find(source.service->nick.c_str());
		Anope::string target = "";
		User *u = source.GetUser();
		
		if (chan.c_str()[0] == '#')
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
			}
		}
		else
		{
			User *u2 = User::Find(chan, true);
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
		if (!message.empty())
		{
			Log(LOG_COMMAND,source, this) << "to ACT out " << message.c_str() << " in " << target.c_str();
			IRCD->SendAction(bi, target.c_str(), message.c_str());
		}
		else {
			this->SendSyntax(source);
			return;
		}
		return;
	}
	bool OnHelp(CommandSource &source, const Anope::string &subcommand) anope_override
	{
		this->SendSyntax(source);
		source.Reply(" \n");
		source.Reply(_("Performs an action in channel\n"
			"Note: This command is ALWAYS logged.\n"
			" \n"));
		return true;
	}
};

class RSAct : public Module
{
	CommandRSAct commandrsact;

 public:
	RSAct(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA), 
		commandrsact(this)
	{
		this->SetAuthor("Azander");
		this->SetVersion("2.0");

	}
};

MODULE_INIT(RSAct)
