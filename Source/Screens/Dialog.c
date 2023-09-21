/****************************/
/*   	DIALOG.C		    */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


extern	float					gCurrentAspectRatio,gGlobalTransparency,gFramesPerSecondFrac;
extern	int						gLevelNum;
extern	FSSpec					gDataSpec;
extern	long					gTerrainUnitWidth,gTerrainUnitDepth;
extern	OGLColorRGB				gGlobalColorFilter;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	u_long					gGlobalMaterialFlags;
extern	SpriteType	*gSpriteGroupList[];
extern	AGLContext		gAGLContext;
extern	PrefsType			gGamePrefs;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void CreateDialogMessage(int messNum, int priority, float duration, OGLPoint3D *fromWhere);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	LETTER_SIZE			9.0f
#define	LETTER_SPACING		(LETTER_SIZE * 1.15f)

#define	DIALOG_ICON_WIDTH	50.0f




#define	DIALOG_FRAME_WIDTH		530

enum
{
	DIALOG_MODE_APPEAR,
	DIALOG_MODE_DISAPPEAR,
	DIALOG_MODE_STAY,
	DIALOG_MODE_NONE
};


static float	gDialogAlpha;


/*********************/
/*    VARIABLES      */
/*********************/

static	int		gDialogMode;

static const char *gTotalDialogString[MAX_LANGUAGES][MAX_DIALOG_MESSAGES] =
{
		/* ENGLISH */
	{
		"Hi Skip!  I'm Sam the Snail, and I seem to have lost my shell.^If you can find it for me I'll give you the key to the gate.",					// DIALOG_MESSAGE_NEEDSNAILSHELL
		"Thanks for finding my shell, Skip!  Here's the key I promised!",									// DIALOG_MESSAGE_GOTSNAILSHELL

		"Hi Skip! If you can put my scarecrow's head back on I'll give^you another key.",					// DIALOG_MESSAGE_FINDSCARECROWHEAD
		"Great! You found the scarecrow's head! Now just put it back^on him and I'll give you the key!",	// DIALOG_MESSAGE_PUTSCARECROWHEAD
		"Thanks for fixing my scarecrow, Skip! Here's your key!",											// DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD

		"Find my marble and use it to knock down these batteries.^I'll give you a key if you can do it!",	// DIALOG_MESSAGE_FINDMARBLE
		"Good job finding my marble, Skip!  Now  kick the marble^to knock down all the batteries.",			// DIALOG_MESSAGE_BOWLMARBLE
		"Yeehaaa!! Good job, Skip!  Here's another key.",													// DIALOG_MESSAGE_DONEBOWLING

		"Well, hello Skip!  I'll give you a map if you'll bring me an acorn.",								// DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN
		"Howdy Skip! I'll trade you this checkpoint for an acorn.",											// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN
		"Skip, this critter looks stuck. See if you can lift the bar by^pressing your pickup button.", 		// DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP
		"Thank you, Skip! Here ya go!", 																	// DIALOG_MESSAGE_CHIPMUNK_THANKS
		"Checkpoint!",																						// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT

		"Skip, the red key is floating on a leaf somewhere in the pool.^But be careful, the chlorine in the water will hurt ya!",	// DIALOG_MESSAGE_POOLWATER

		"If you can squash all of the berries along the trail I'll give^you a key at the end!",				// DIALOG_MESSAGE_SMASHBERRIES
		"Hey, you missed some berries, Skip.  Go back and squish all of^them.",								// DIALOG_MESSAGE_SQUISHMORE
		"You made quite a mess back there!  Here's they key!",												// DIALOG_MESSAGE_SQUISHDONE

		"Skip, this pipe will take you inside the house. I'll let you in if^you can get all the ticks and fleas off my friend Fido.",	// DIALOG_MESSAGE_DOGHOUSE

		"You got all the fleas! Now get those ticks!",														// DIALOG_MESSAGE_GOTFLEAS
		"That's all the ticks! Now finish off the fleas!!",													// DIALOG_MESSAGE_GOTTICKS
		"Good job! You've made Fido a happy dog!",															// DIALOG_MESSAGE_HAPPY DOG
		"Remember Skip, if you'll get all the ticks and fleas off of my^friend Fido, I'll let you into the drain pipe.",	// DIALOG_MESSAGE_REMEMBERDOG

		"The sewer will take you inside the house. Good luck, Skip!",										// DIALOG_MESSAGE_PLUMBINGINTRO

		"If you can beat Sam in a slot-car race I'll give you this key.^So hop on the car and race!",		// DIALOG_MESSAGE_SLOTCAR
		"Skip, there are mice in the yard who are caught in traps. If you^can rescue at least 4 of them I'll give you another key!",	// DIALOG_MESSAGE_RESCUEMICE
		"I'll give you a key when you free 4 mice from their traps!",										// DIALOG_MESSAGE_RESCUEMICE2
		"Thanks for rescuing those mice, Skip!",															// DIALOG_MESSAGE_MICESAVED

		"You won, Skip!  Here ya go!",																		// DIALOG_MESSAGE_SLOTCARPLAYERWON
		"Aw, you can do better than that, Skip. Hop back on and try again!",								// DIALOG_MESSAGE_SLOTCARTRYAGAIN

		"Hey, Skip! If you can help me finish this puzzle I'll give you^a key.",							// DIALOG_MESSAGE_DOPUZZLE
		"Thanks for finishing the puzzle for me, Skip! Here's a key for ya!",								// DIALOG_MESSAGE_DONEPUZZLE

		"If you can destroy all of the Ant Hills I'll tell you which way^Bully Bee went! To destroy an Ant Hill drop a bomb down the^hole.",	// DIALOG_MESSAGE_BOMBHILLS
		"To drop a bomb press the Pickup-Drop button. To shoot at^Dragonflies, press the Kick button.", 	// DIALOG_MESSAGE_BOMBHILLS
		"Thanks for destroying all the Ant Hills, Skip! Last time I saw^Bully Bee he was headed toward the pond!", // DIALOG_MESSAGE_HILLSDONE

		"Hey Skip!  These balls seem to scare off the Moths.", 												// DIALOG_MESSAGE_MOTHBALL
		"Skip, this door won't open. It looks like it needs a few computer^chips and a battery.", 			// DIALOG_MESSAGE_SILICONDOOR

		"Skip, I'll give you a key if you bring me all of the red clovers!^They're all up above on those hangers.", // DIALOG_MESSAGE_GETREDCLOVERS
		"Thanks for getting all the red clovers, Skip! Here's the key!", 									// DIALOG_MESSAGE_GOTREDCLOVERS

		"Aloha Skip! Help the fisherman catch some fish by jumping on^his fishing bobbers! If you can catch 4 fish I'll give you a key.", 	// DIALOG_MESSAGE_GOFISHING
		"You're doing great! Keep catching fish!",									// DIALOG_MESSAGE_MOREFISH
		"You're quite the Angler, Skip! Here's a key for ya!",						// DIALOG_MESSAGE_THANKSFISH
		"Those pesky Ants have stolen all the food! I'll give you a key if^you'll bring back 6 pieces of food for me. Just drop them anywhere^on the picnic basket.", 	// DIALOG_MESSAGE_GETFOOD
		"Keep bringin' me food, Skip!",												// DIALOG_MESSAGE_MOREFOOD
		"Mmmmm, yummy! Thanks for the food, Skip!  Here's the key!",				// DIALOG_MESSAGE_THANKSFOOD

		"Skip, Bully Bee took your bag inside the Bee Hive! The only way^I know to make the Bees go away is to smoke them out, so gather^some leaves and twigs under the hive.",				// DIALOG_MESSAGE_GETKINDLING
		"That's a good start, but it's not enough kindling to make a big fire.^Gather more leaves and twigs under the hive!",	// DIALOG_MESSAGE_MOREKINDLING
		"That looks good, Skip! Now we just need a spark to light the^fire!",			// DIALOG_MESSAGE_LIGHTFIRE
		"Great job, Skip! The hive now looks safe to enter!",			// DIALOG_MESSAGE_ENTERHIVE
		"Hey, there's a key on top of the bottle. You'll need to smash^it to get it.",						// DIALOG_MESSAGE_BOTTLEKEY

		"Skip, water from the gutter is flooding the garbage can, and there^are 9 mice who need your help! Rescue them before they drown^and I'll give you a key.", // DIALOG_MESSAGE_MICEDROWN
		"Good job freeing those Mice, Skip!  Here's the key!",			//	DIALOG_MESSAGE_THANKSNODROWN
		"I think your best way out of here is to fly this plane, but^it's missing some parts. I bet if you put the propeller and^wheels back on it it'll fly!", // DIALOG_MESSAGE_GLIDER
		"Hey Skip, these soda cans are still full. I bet you can pop the^tab on them!",						// DIALOG_MESSAGE_SODACAN
	},

		/* FRENCH */
	{
		"Hé Skip!  Je suis Sam l'Escargot, et je crois que j'ai perdu ma^coquille. Si tu réussis à me la retrouver, je te donnerai^la clé de la porte.",	// DIALOG_MESSAGE_NEEDSNAILSHELL
		"Merci d'avoir retrouvé ma coquille, Skip!  Voilà la clé que je^t'ai promise!",						// DIALOG_MESSAGE_GOTSNAILSHELL

		"Hé Skip! Peux-tu retrouver la tête de mon épouvantail, je te^donnerai une autre clé.",					// DIALOG_MESSAGE_FINDSCARECROWHEAD
		"Super! Tu l'as retrouvée! Remets-la en place sur les épaules de^mon épouvantail et je te donnerai la clé promise!",	// DIALOG_MESSAGE_PUTSCARECROWHEAD
		"Merci d'avoir réparé mon épouvantail, Skip! Voilà ta clé!",											// DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD

		"Trouve ma bille et utilise la pour renverser ces batteries.^Je te donnerai une clé si tu réussis à le faire!",	// DIALOG_MESSAGE_FINDMARBLE
		"Bon travail, tu as trouvé ma bille, Skip! Maintenant, donne^un coup de pied dans la bille pour renverser toutes les^batteries.",			// DIALOG_MESSAGE_BOWLMARBLE
		"Yeeh!! Bon travail, Skip!  Voilà une autre clé.",													// DIALOG_MESSAGE_DONEBOWLING

		"Salut Skip!  Tu sais, je te donnerai une carte si tu^m'apportes un gland.",						// DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN
		"Salut Skip, ça va! Je suis prêt à t'échanger ce témoin^de passage contre un gland.",				// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN
		"Skip, cette bestiole a l'air coincée. Essaye de la libérer^en soulevant cette barre en pressant le bouton ramasser.",		// DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP
		"Merci Skip! Tu as réussi!", 																		// DIALOG_MESSAGE_CHIPMUNK_THANKS
		"Et un témoin de passage!",																			// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT

		"Skip, la clé rouge est sur l'une des feuilles qui flottent^dans la piscine. Mais fais bien attention, le chlore contenu dans^l'eau pourrait bien te blesser!",	// DIALOG_MESSAGE_POOLWATER

		"Si tu réussis à ramasser et à écraser toutes les baies qui^se trouvent sur ton chemin, Je te donnerai une clé quand tu^reviendras!",	// DIALOG_MESSAGE_SMASHBERRIES
		"Hé, tu en as raté quelques-unes unes, Skip. Tu dois^repartir, les trouver toutes et les écraser.",	// DIALOG_MESSAGE_SQUISHMORE
		"Tu as vraiment fait un joli carnage sur ton chemin!^Tiens, voilà ta clé!",						// DIALOG_MESSAGE_SQUISHDONE

		"Skip, cette conduite te mènera à l'intérieur de la maison.^Je te laisserai entrer si tu réussis à débarrasser mon ami^Fido de toutes ses tiques et ses puces.",	// DIALOG_MESSAGE_DOGHOUSE

		"Tu l'as débarrassé de toutes ses puces! Il ne te reste^plus qu'a le débarrasser de ses tiques!",	// DIALOG_MESSAGE_GOTFLEAS
		"Tu l'as débarrassé de toutes ses tiques! Il ne te reste^plus qu'à le débarrasser de toutes ses puces!!",	// DIALOG_MESSAGE_GOTTICKS
		"Bon travail! Grâce à toi, Fido est enfin un^chien heureux!",										// DIALOG_MESSAGE_HAPPY DOG
		"Rappelle-toi bien, Skip, si tu réussis à débarrasser mon ami^Fido de toutes ses tiques et de toutes ses puces, je te^laisserai rentrer dans la conduite d'eau.",	// DIALOG_MESSAGE_REMEMBERDOG

		"La conduite de cet égout va t'emmener directement à^l'intérieur de la maison. Bonne chance, Skip!",	// DIALOG_MESSAGE_PLUMBINGINTRO

		"Si tu réussis à battre Sam dans une course de petites^voitures, je te donnerai cette clé. Alors en voiture et que^le meilleur gagne!",	// DIALOG_MESSAGE_SLOTCAR
		"Skip, des souris sont prises au piège dans la cour.^Si tu réussis à en libérer au moins 4 des trappes dont elles^sont prisonnières, je te donnerai une autre clé!",	// DIALOG_MESSAGE_RESCUEMICE
		"Je te donnerai une clé chaque fois que tu auras réussi à^libérer 4 souris des trappes dont elles sont prisonnières!",	// DIALOG_MESSAGE_RESCUEMICE2
		"Merci d'avoir libéré ces souris, Skip!",															// DIALOG_MESSAGE_MICESAVED

		"Tu as gagné, Skip!  Tu peux continuer ton chemin!",												// DIALOG_MESSAGE_SLOTCARPLAYERWON
		"Ho, tu peux faire bien mieux que ça, Skip. Allez,^ouste, recommence!",								// DIALOG_MESSAGE_SLOTCARTRYAGAIN

		"Hé, Skip! Si tu m'aides à terminer ce puzzle, je te^donnerai une clé.",							// DIALOG_MESSAGE_DOPUZZLE
		"Merci d'avoir terminé ce puzzle pour moi, Skip!^Voilà ta clé!",								// DIALOG_MESSAGE_DONEPUZZLE

		"Si tu réussis à détruire toutes les Fourmilières, je te dirai dans^quelle direction est partie la Bully Bee! Tu dois lâcher une^bombe dans le trou de la Fourmilière pour la détruire.",	// DIALOG_MESSAGE_BOMBHILLS
		"Pour lâcher une bombe, tu dois presser le bouton Ramasser-Laisser^Tomber. Pour atteindre les Libellules, tu dois^presser le bouton Donner un Coup de Pied.", 	// DIALOG_MESSAGE_BOMBHILLS
		"Merci d'avoir détruit toutes les Fourmilières, Skip! La dernière^fois que j'ai vu cette méchante Bully Bee, elle se dirigeait^vers l'étang!", // DIALOG_MESSAGE_HILLSDONE

		"Hé, Skip! Ces boules semblent faire une telle peur aux Mites^qu'elles s'enfuient à tire-d'aile.", // DIALOG_MESSAGE_MOTHBALL
		"Skip, cette porte refuse de s'ouvrir. Il faudrait trouver^quelques puces électroniques et une batterie.", 			// DIALOG_MESSAGE_SILICONDOOR

		"Skip, je te donnerai une clé si tu me rapportes tous les^trèfles rouges! Ils sont tous là haut sur ces cintres.", // DIALOG_MESSAGE_GETREDCLOVERS
		"Merci de m'avoir rapporté tous les trèfles rouges, Skip!^Voilà la clé!", 									// DIALOG_MESSAGE_GOTREDCLOVERS

		"Aloha Skip! Aide donc ce pêcheur à attraper du poisson en sautant^sur les flotteurs de ses lignes! Si tu réussis à attraper 4 poissons,^je te donnerai une clé.", 	// DIALOG_MESSAGE_GOFISHING
		"Tu es vraiment un spécialiste! Continue ta pêche miraculeuse!",						// DIALOG_MESSAGE_MOREFISH
		"Tu es vraiment un sacré pêcheur, Skip! Voilà ta clé!",								// DIALOG_MESSAGE_THANKSFISH
		"Ces satanées Fourmis ont volé toute ma nourriture! Je te donnerai^une clé si tu me rapportes 6 choses à manger. Tu n'auras qu'à^les laisser tomber dans ce panier à pique-nique.", 	// DIALOG_MESSAGE_GETFOOD
		"Continue à m'amener à manger, Skip!",											// DIALOG_MESSAGE_MOREFOOD
		"Mmmmm, miammm! Merci pour ce repas de roi, Skip^Voilà la clé!",				// DIALOG_MESSAGE_THANKSFOOD

		"Skip, Bully Bee a emmené ton sac dans la Ruche! La seule façon d'en^faire sortir les Abeilles est de les enfumer, ramasse des^feuilles et des brindilles et dépose-les sous la ruche.",				// DIALOG_MESSAGE_GETKINDLING
		"C'est un bon départ, mais il n'y a pas encore assez de petit bois^pour faire un bon feu. Rajoute d'autres feuilles et^d'autres brindilles sous la ruche!",	// DIALOG_MESSAGE_MOREKINDLING
		"Ca me semble parfait, Skip! Il ne nous manque plus qu'une^étincelle pour allumer ce feu!",			// DIALOG_MESSAGE_LIGHTFIRE
		"Bon travail, Skip! Je pense que tu peux maintenant pénétrer^dans la ruche en toute sécurité!",		// DIALOG_MESSAGE_ENTERHIVE
		"Hé, il y a une clé sur cette bouteille. Tu vas devoir la casser^pour récupérer cette clé.",		// DIALOG_MESSAGE_BOTTLEKEY

		"Skip, l'eau qui coule de la gouttière a inondé la poubelle et^9 souris attendent ton secours! Sauve-les avant qu'elles^se noient et je te donnerai une clé.",  // DIALOG_MESSAGE_MICEDROWN
		"Tu as fait du bon travail en sauvant ces Souris, Skip!^Voilà la clé!",			//	DIALOG_MESSAGE_THANKSNODROWN
		"Je pense que le meilleur moyen de s'enfuir d'ici est d'utiliser^cet avion, mais il manque des pièces. Je parie qu'il fonctionnera si tu^réussis à remonter cette hélice et ces roues!",  // DIALOG_MESSAGE_GLIDER
		"Hé, Skip, ces boîtes de soda sont encore pleines.^Je parie que tu peux les ouvrir!",					// DIALOG_MESSAGE_SODACAN
	},

		/* GERMAN */
	{
		"Hallo Skip! Ich bin Sam die Schnecke und ich scheine mein Haus^verloren zu haben. Wenn Du es für mich findest, gebe ich Dir^den Schlüssel für das Tor.",	// DIALOG_MESSAGE_NEEDSNAILSHELL
		"Danke, daß Du mein Haus gefunden hast, Skip! Hier ist der^versprochene Schlüssel!",				// DIALOG_MESSAGE_GOTSNAILSHELL

		"Hallo Skip! Wenn Du meiner Vogelscheuche wieder ihren Kopf^aufsetzen kannst, gebe ich Dir einen weiteren Schlüssel.",					// DIALOG_MESSAGE_FINDSCARECROWHEAD
		"Großartig! Du hast den Kopf der Vogelscheuche gefunden! Setze^ihn ihr nun wieder auf und dann gebe ich Dir den Schlüssel!",	// DIALOG_MESSAGE_PUTSCARECROWHEAD
		"Danke, daß Du meine Vogelscheuche repariert hast, Skip!^Hier ist Dein Schlüssel!",											// DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD

		"Finde meine Murmel und verwende sie, um diese Batterien^umzuschmeißen. Ich gebe Dir einen Schlüssel, wenn Du das^machen kannst!",	// DIALOG_MESSAGE_FINDMARBLE
		"Gut gemacht, Skip, Du hast meine Murmel gefunden! Nun trete^die Murmel, um alle Batterien umzuschmeißen.",			// DIALOG_MESSAGE_BOWLMARBLE
		"Juchhuu! Gute Arbeit, Skip! Hier hast Du einen weiteren^Schlüssel.",													// DIALOG_MESSAGE_DONEBOWLING

		"Nun, hallo Skip! Ich gebe Dir eine Karte, wenn Du mir eine^Eichel bringst.",								// DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN
		"Tag Skip! Ich gebe Dir diesen Kontrollpunkt für eine Eichel.",											// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN
		"Skip, dieses Wesen scheint festzustecken. Schau, ob Du durch^Drücken des Aufheben-Knopfes die Klammer hochheben kannst.", 		// DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP
		"Danke, Skip! Hier hast Du sie!", 																	// DIALOG_MESSAGE_CHIPMUNK_THANKS
		"Kontrollpunkt!",																						// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT

		"Skip, der rote Schlüssel schwimmt auf einem Blatt irgendwo im^Pool. Aber sei vorsichtig, das Chlor im Wasser wird Dich^verletzen!",	// DIALOG_MESSAGE_POOLWATER

		"Wenn Du alle Beeren entlang der Strecke zerquetschst, gebe ich^Dir am Ende einen Schlüssel!",				// DIALOG_MESSAGE_SMASHBERRIES
		"Hey, Du hast ein paar Beeren vergessen, Skip. Gehe zurück und^zerquetsche alle.",								// DIALOG_MESSAGE_SQUISHMORE
		"Du hast da eine ganz schöne Sauerei hinterlassen!^Hier ist der Schlüssel!",												// DIALOG_MESSAGE_SQUISHDONE

		"Skip, dieses Rohr bringt Dich ins Haus. Ich lasse Dich hinein,^wenn Du meinen Freund Fido von allen Zecken und Flöhen^befreien kannst.",	// DIALOG_MESSAGE_DOGHOUSE

		"Du hast alle Flöhe erwischt! Nun kümmere Dich um die Zecken!",														// DIALOG_MESSAGE_GOTFLEAS
		"Du hast alle Zecken erwischt! Nun kümmere Dich um die Flöhe!",													// DIALOG_MESSAGE_GOTTICKS
		"Gute Arbeit! Du hast Fido zu einem glücklichen Hund gemacht!",															// DIALOG_MESSAGE_HAPPY DOG
		"Denke dran, Skip, wenn Du meinen Freund Fido von allen Flöhen^und Zecken befreien kannst, lasse ich Dich ins Abwasserrohr.",	// DIALOG_MESSAGE_REMEMBERDOG

		"Das Abwasserrohr bringt Dich ins Haus, Viel Glück, Skip!",										// DIALOG_MESSAGE_PLUMBINGINTRO

		"Wenn Du Sam im Autorennen besiegen kannst, gebe ich Dir diesen^Schlüssel. Hüpfe aufs Auto und rase los!",		// DIALOG_MESSAGE_SLOTCAR
		"Skip, es gibt Mäuse hier, die in Fallen gefangen sind, Wenn Du^mindestens 4 davon befreien kannst, gebe ich Dir einen weiteren Schlüssel!",	// DIALOG_MESSAGE_RESCUEMICE
		"Ich gebe Dir einen Schlüssel, wenn Du 4 Mäuse aus den Fallen^befreist!",										// DIALOG_MESSAGE_RESCUEMICE2
		"Danke für das Befreien der Mäuse, Skip!",															// DIALOG_MESSAGE_MICESAVED

		"Du hast gewonnen, Skip! Weiter so!",																		// DIALOG_MESSAGE_SLOTCARPLAYERWON
		"Ach, das kannst Du besser, Skip. Hüpf nochmal auf und versuche^es erneut!",								// DIALOG_MESSAGE_SLOTCARTRYAGAIN

		"Hey, Skip! Wenn Du mir beim Lösen des Puzzles hilfst, gebe ich^Dir einen Schlüssel!",							// DIALOG_MESSAGE_DOPUZZLE
		"Danke, daß Du das Puzzle für mich gelöst hast, Skip! Hier ist^ein Schlüssel für Dich!",								// DIALOG_MESSAGE_DONEPUZZLE

		"Wenn Du alle Ameisenhügel zerstörst, sage ich Dir, wohin^Tyrannen-Biene geflogen ist! Um einen Ameisenhügel zu^zerstören, lasse eine Bombe ins Loch fallen.",	// DIALOG_MESSAGE_BOMBHILLS
		"Um eine Bombe fallenzulassen, drücke die Aufheben-Fallenlassen-^Taste. Um auf Libellen zu schießen, drücke die Treten-Taste", 	// DIALOG_MESSAGE_BOMBHILLS
		"Danke für das Zerstören der Ameisenhügel, Skip! Als ich^Tyrannen-Biene das letzte Mal gesehen habe, flog sie Richtung^Teich!", // DIALOG_MESSAGE_HILLSDONE

		"Hey Skip! Diese Kugeln scheinen die Motten zu vertreiben.", 												// DIALOG_MESSAGE_MOTHBALL
		"Skip, diese Tür öffnet sich nicht. Es sieht so aus, als benötigt^sie ein paar Computer-Chips und eine Batterie.", 			// DIALOG_MESSAGE_SILICONDOOR

		"Skip, ich gebe Dir einen Schlüssel, wenn Du mir alle roten^Kleeblätter bringst! Sie befinden sich alle^oben auf den Kleiderbügeln.", // DIALOG_MESSAGE_GETREDCLOVERS
		"Danke, daß Du mir alle roten Kleeblätter gebracht hast, Skip!^Hier ist der Schlüssel!", 								// DIALOG_MESSAGE_GOTREDCLOVERS

		"Aloha Skip! Hilf dem Fischer, einige Fische zu fangen, indem Du^auf die Angelhaken hüpfst. Wenn Du 4 Fische fangen kannst, gebe ich^Dir einen Schlüssel.", 	// DIALOG_MESSAGE_GOFISHING
		"Das machst Du gut! Fange weiterhin Fische!",									// DIALOG_MESSAGE_MOREFISH
		"Du bist ein guter Angler, Skip! Hier ist ein Schlüssel für Dich!",						// DIALOG_MESSAGE_THANKSFISH
		"Diese blöden Ameisen haben das ganze Essen geklaut! Ich gebe Dir^einen Schlüssel, wenn Du mir 6 Essens-Stücke zurückbringst.^Lege sie irgendwo auf dem Picknick-Korb ab.", 	// DIALOG_MESSAGE_GETFOOD
		"Bringe mir weiter Essen, Skip!",												// DIALOG_MESSAGE_MOREFOOD
		"Mmmmm, lecker! Danke für das Essen, Skip!  Hier ist der Schlüssel",				// DIALOG_MESSAGE_THANKSFOOD

		"Skip, Tyrannen-Biene hat Deinen Rucksack in den Bienenkorb gebracht!^Du kannst die Bienen nur durch Ausräuchern vertreiben, sammele^also etwas Laub und Zweige unter dem Korb.",				// DIALOG_MESSAGE_GETKINDLING
		"Nicht schlecht, aber nicht zündend genug, um ein großes Feuer zu^machen. Sammele mehr Laub und Zweige unter dem Korb!",	// DIALOG_MESSAGE_MOREKINDLING
		"Das sieht gut aus, Skip! Nun benötigen wir nur noch einen^Funken, um das Feuer zu entzünden!",			// DIALOG_MESSAGE_LIGHTFIRE
		"Gute Arbeit, Skip! Den Korb kann man nun gefahrlos betreten!",			// DIALOG_MESSAGE_ENTERHIVE
		"Hey, da ist ein Schlüssel oben auf der Flasche. Du mußt sie^zuerst zerstören, um ihn zu bekommen.",						// DIALOG_MESSAGE_BOTTLEKEY

		"Skip, aus der Regenrinne fließt Wasser in den Mülleimer und es^gibt 9 Mäuse, die Deine Hilfe benötigen! Rette sie, bevor sie^ertrinken, dann gebe ich Dir einen Schlüssel.", // DIALOG_MESSAGE_MICEDROWN
		"Gute Arbeit beim Befreien der Mäuse, Skip! Hier ist der^Schlüssel!",			//	DIALOG_MESSAGE_THANKSNODROWN
		"Ich denke, der beste Weg hier raus ist, dieses Flugzeug zu^fliegen, es fehlen aber ein paar Teile. Wetten, es fliegt, wenn Du^den Propeller und die Räder wieder dran machst?", // DIALOG_MESSAGE_GLIDER
		"Hey Skip, diese Cola-Dosen sind immer noch voll. Ich wette,^Du kannst auf ihnen den Öffner ziehen!",						// DIALOG_MESSAGE_SODACAN
	},

		/* SPANISH */
	{
		"¡Hola Skip! Soy Sam el caracol y parece que he perdido mi^caparazón. Si lo encuentras, te daré la llave de la puerta.",					// DIALOG_MESSAGE_NEEDSNAILSHELL
		"¡Gracias por encontrar mi caparazón, Skip! ¡Aquí tienes la llave^prometida!",									// DIALOG_MESSAGE_GOTSNAILSHELL

		"¡Hola Skip! Si consigues volver a colocar la cabeza del^espantapájaros te daré otra llave.",					// DIALOG_MESSAGE_FINDSCARECROWHEAD
		"¡Genial! ¡Has encontrado la cabeza del espantapájaros!^¡Vuelve a colocarla y te daré la llave!",	// DIALOG_MESSAGE_PUTSCARECROWHEAD
		"¡Gracias por arreglar mi espantapájaros, Skip! ¡Toma la llave!",											// DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD

		"Busca mi canica y úsala para derribar todas estas baterías.^¡Te daré una llave si lo consigues!",	// DIALOG_MESSAGE_FINDMARBLE
		"¡Fantástico, encontraste la canica, Skip! Ahora dale una^patada para derribar todas las baterías.",			// DIALOG_MESSAGE_BOWLMARBLE
		"¡Así se hace! ¡Buen trabajo, Skip! Toma, otra llave más.",													// DIALOG_MESSAGE_DONEBOWLING

		"¡Hombre, hola Skip! Te daré un mapa si me traes una bellota.",								// DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN
		"¡Hombre Skip! Te cambio este punto de control por una bellota.",											// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN
		"Skip, este bicho está atrapado. Intenta levantar la barra^pulsando el botón Coger.", 		// DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP
		"¡Gracias, Skip! ¡Aquí tienes!", 																	// DIALOG_MESSAGE_CHIPMUNK_THANKS
		"¡Un punto de control!",																						// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT

		"Skip, la llave roja está flotando en el estanque. ¡Ten cuidado,^el cloro del agua te hará daño!",	// DIALOG_MESSAGE_POOLWATER

		"¡Si logras espachurrar todas las bayas del camino te daré^una llave!",				// DIALOG_MESSAGE_SMASHBERRIES
		"Te faltan algunas bayas, Skip. Vuelve y aplástalas todas.",								// DIALOG_MESSAGE_SQUISHMORE
		"¡Menuda la que has armado! ¡Toma la llave!",												// DIALOG_MESSAGE_SQUISHDONE

		"Skip, entrarás a la casa por esta tubería. Te dejaré entrar si^le quitas a mi amigo Fido todas la garrapatas y pulgas.",	// DIALOG_MESSAGE_DOGHOUSE

		"¡Has acabado con las pulgas! ¡Ahora a por las garrapatas!",														// DIALOG_MESSAGE_GOTFLEAS
		"¡Ya no quedan garrapatas! ¡Ahora quítale las pulgas!",													// DIALOG_MESSAGE_GOTTICKS
		"¡Bien hecho! ¡Mira qué contento está Fido!",															// DIALOG_MESSAGE_HAPPY DOG
		"Recuerda Skip, si le quitas a mi amigo Fido todas las garrapatas^y las pulgas te dejaré entrar por la cañería.",	// DIALOG_MESSAGE_REMEMBERDOG

		"Podrás entrar en la casa por la alcantarilla. ¡Buena suerte,^Skip!",										// DIALOG_MESSAGE_PLUMBINGINTRO

		"¡Si ganas a Sam en una carrera al escalextrix te daré esta^llave. Venga, sube al coche y ¡a correr!",		// DIALOG_MESSAGE_SLOTCAR
		"Skip, hay varios ratones en el patio atrapados en trampas. ¡Si^logras rescatar por lo menos a 4 te daré otra llave!",	// DIALOG_MESSAGE_RESCUEMICE
		"¡Te daré una llave si consigues liberar a 4 ratones de las^trampas!",										// DIALOG_MESSAGE_RESCUEMICE2
		"¡Gracias por rescatar a los ratones, Skip!",															// DIALOG_MESSAGE_MICESAVED

		"¡Has ganado, Skip! ¡Toma la llave!",																		// DIALOG_MESSAGE_SLOTCARPLAYERWON
		"Venga, Skip, seguro que puedes hacerlo mejor. ¡Sube al coche^y vuelve a intentarlo!",								// DIALOG_MESSAGE_SLOTCARTRYAGAIN

		"¡Hey, Skip! Si me ayudas a terminar este puzzle te doy una^llave.",							// DIALOG_MESSAGE_DOPUZZLE
		"¡Gracias por terminar el puzzle, Skip! ¡Esta llave es para ti!",								// DIALOG_MESSAGE_DONEPUZZLE

		"¡Destruye todos los hormigueros y te diré por dónde se ha ido^la Abeja Mala! Para destruir un hormiguero, lanza una bomba por el^agujero.",	// DIALOG_MESSAGE_BOMBHILLS
		"Para lanzar la bomba pulsa el botón Coger/Soltar. Para disparar^a las libélulas, pulsa el botón Patada.", 	// DIALOG_MESSAGE_BOMBHILLS
		"¡Gracias por destruir todos los hormigueros, Skip! ¡La última vez^que ví a la Abeja Mala se dirigía al estanque!", // DIALOG_MESSAGE_HILLSDONE

		"¡Oye Skip! Parece que esas bolas asustan a las polillas.", 												// DIALOG_MESSAGE_MOTHBALL
		"Skip, esta puerta no se abre. Creo que necesita unos cuantos^chips y una batería.", 			// DIALOG_MESSAGE_SILICONDOOR

		"¡Skip, te daré una llave si me traes todos los tréboles rojos!^Están todos por encima de las perchas.", // DIALOG_MESSAGE_GETREDCLOVERS
		"¡Gracias por traerme todos los tréboles rojos, Skip! ¡Aquí^tienes la llave!", 									// DIALOG_MESSAGE_GOTREDCLOVERS

		"¡Aloha Skip! ¡Ayuda al pescador a pescar abalanzándote sobre las^boyas! Si logras atrapar 4 peces te daré una llave.", 	// DIALOG_MESSAGE_GOFISHING
		"¡Lo haces muy bien! ¡Sigue pescando!",									// DIALOG_MESSAGE_MOREFISH
		"¡Estás hecho todo un pescador, Skip! ¡Toma tu llave!",						// DIALOG_MESSAGE_THANKSFISH
		"¡Esas molestas hormigas me han robado la comida! Te daré una llave si^me traes 6 trozos de comida. Deja los trozos en la cesta de^la merienda.", 	// DIALOG_MESSAGE_GETFOOD
		"¡Sigue trayendo comida, Skip!",												// DIALOG_MESSAGE_MOREFOOD
		"¡Mmmmm, qué rico! ¡Gracias por la comida, Skip! ¡Toma la llave!",				// DIALOG_MESSAGE_THANKSFOOD

		"¡Skip, la Abeja Mala se ha llevado tu mochila a la colmena! Sólo^conseguirás hacer salir a las abejas con humo, así que^amontona hojas y ramitas debajo de la colmena.",				// DIALOG_MESSAGE_GETKINDLING
		"Vas bien, pero no es suficiente como para hacer una buena^hoguera. ¡Coloca más hojas y ramitas bajo la colmena!",	// DIALOG_MESSAGE_MOREKINDLING
		"¡Fenomenal, Skip! ¡Ahora sólo necesitas una chispa para encender^el fuego!",			// DIALOG_MESSAGE_LIGHTFIRE
		"¡Buen trabajo, Skip! ¡Ya puedes entrar tranquilo en la colmena!",			// DIALOG_MESSAGE_ENTERHIVE
		"Mira, hay una llave encima de la botella. Tendrás que romperla^para conseguirla.",						// DIALOG_MESSAGE_BOTTLEKEY

		"¡Skip, el agua de la cañería está inundando la papelera y^hay 9 ratones que necesitan tu ayuda! Rescátalos antes de que^se ahoguen y te daré una llave.", // DIALOG_MESSAGE_MICEDROWN
		"¡Bien hecho, liberaste a los ratones, Skip! ¡Aquí tienes tu^llave!",			//	DIALOG_MESSAGE_THANKSNODROWN
		"Creo que la mejor forma de escapar es pilotando este avión,^pero le faltan algunas piezas. ¡Seguro que si colocas la hélice^y las ruedas podrá volar!", // DIALOG_MESSAGE_GLIDER
		"Mira Skip, esas latas de refresco siguen llenas.^¡Seguro que puedes abrir las anillas!",						// DIALOG_MESSAGE_SODACAN
	},

		/* ITALIAN */
	{
		"Ciao Skip! Sono Sam la lumaca e credo di aver perso il mio^guscio. Se me lo riporti, ti daró la chiave per il cancello.",					// DIALOG_MESSAGE_NEEDSNAILSHELL
		"Grazie per aver trovato il mio guscio, Skip! Ecco la chiave^che ti ho promesso!",					// DIALOG_MESSAGE_GOTSNAILSHELL

		"Ciao Skip! Se puoi rimettere al suo posto la testa dello^spaventapasseri, ti daró un'altra chiave.",	// DIALOG_MESSAGE_FINDSCARECROWHEAD
		"Bravo! Hai trovato la testa dello spaventapasseri!^Ora rimettila al suo posto e ti daró la chiave!",	// DIALOG_MESSAGE_PUTSCARECROWHEAD
		"Grazie per aver sistemato il mio spaventapasseri, Skip!^Ecco la tua chiave!",											// DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD

		"Trova la mia biglia e usala per abbattere queste pile.^Se riesci, ti daró una chiave!",	// DIALOG_MESSAGE_FINDMARBLE
		"Ben fatto! Hai trovato la biglia, Skip! Ora, usala per^abbattere tutte le pile, dandole un calcio.",			// DIALOG_MESSAGE_BOWLMARBLE
		"Bravo!! Ben fatto, Skip! Eccoti un'altra chiave.",													// DIALOG_MESSAGE_DONEBOWLING

		"Salve, Skip! Ti daró una mappa se mi porti una ghianda.",								// DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN
		"Come va Skip? In cambio di una ghianda, pianteró un marcatore.",											// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN
		"Skip, questo topolino sembra intrappolato. Vedi se puoi alzare^la barra usando il tasto di presa.", 		// DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP
		"Grazie Skip! Vai cosí!", 																	// DIALOG_MESSAGE_CHIPMUNK_THANKS
		"Marcatore!",																						// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT

		"Skip, la chiave rossa galleggia su una foglia nella piscina.^Ma stai attento, il cloro nell'acqua ti danneggerà!",	// DIALOG_MESSAGE_POOLWATER

		"Se riesci a schiacciare tutte le bacche lungo il percorso,^alla fine ti daró una chiave!",				// DIALOG_MESSAGE_SMASHBERRIES
		"Ehi, ti sei dimenticato qualche bacca, Skip.^Torna indietro e schiacciale tutte!",								// DIALOG_MESSAGE_SQUISHMORE
		"Hai fatto un bel macello, là dietro! Eccoti la chiave!",												// DIALOG_MESSAGE_SQUISHDONE

		"Skip, questo tubo ti porterà dentro la casa. Ti lasceró^entrare se puoi liberare il mio amico Fido da^tutte le pulci e gli acari.",	// DIALOG_MESSAGE_DOGHOUSE

		"Hai preso tutte le pulci! Ora elimina tutti gli acari!",														// DIALOG_MESSAGE_GOTFLEAS
		"Gli acari sono eliminati! Ora finisci con le pulci!",													// DIALOG_MESSAGE_GOTTICKS
		"Ottimo lavoro! Hai reso Fido un cagnolino felice!",															// DIALOG_MESSAGE_HAPPY DOG
		"Ricorda, Skip, se liberi il mio amico Fido da tutte le pulci^e gli acari, ti lasceró entrare nel tubo di scarico.",	// DIALOG_MESSAGE_REMEMBERDOG

		"Lo scarico ti porterà dentro la casa. Buona fortuna, Skip!",										// DIALOG_MESSAGE_PLUMBINGINTRO

		"Se superi Sam nella gara con le automobiline, ti daró questa^chiave. Salta quindi sull'auto e gareggia con lui!",		// DIALOG_MESSAGE_SLOTCAR
		"Skip, nel recinto ci sono dei topolini presi in trappola.^Se ne liberi 4, ti daró un'altra chiave!",	// DIALOG_MESSAGE_RESCUEMICE
		"Ti daró una chiave quando avrai liberato 4 topolini dalle^loro trappole!",										// DIALOG_MESSAGE_RESCUEMICE2
		"Grazie Skip, per aver liberato i topolini!",															// DIALOG_MESSAGE_MICESAVED

		"Hai vinto, Skip! Bravissimo!",																		// DIALOG_MESSAGE_SLOTCARPLAYERWON
		"Ehm, puoi fare di meglio, Skip. Saltaci su di nuovo e prova ancora!",								// DIALOG_MESSAGE_SLOTCARTRYAGAIN

		"Ehi Skip!	Se mi aiuti a finire questo puzzle, ti daró una^chiave.",							// DIALOG_MESSAGE_DOPUZZLE
		"Grazie per aver completato il puzzle, Skip! Ecco qua una^chiave!",								// DIALOG_MESSAGE_DONEPUZZLE

		"Se riesci a distruggere tutti i formicai, ti diró dov'è andato^Bully Bee! Per distruggere un formicaio, sgancia un bomba nel buco.",	// DIALOG_MESSAGE_BOMBHILLS
		"Per sganciare una bomba, usa il tasto Prendi. Per sparare^a una libellula, usa il tasto per dare un Calcio.", 	// DIALOG_MESSAGE_BOMBHILLS
		"Grazie per aver distrutto tutti i formicai, Skip! L'ultima volta^che ho visto Bully Bee, stava andando verso lo stagno!", // DIALOG_MESSAGE_HILLSDONE

		"Ehi Skip! Le falene sembrano terrorizzate da certe palline!", 												// DIALOG_MESSAGE_MOTHBALL
		"Skip, questa porta non si apre. Sembra che ci voglia qualche^chip di computer e una batteria.", 			// DIALOG_MESSAGE_SILICONDOOR

		"Skip, se mi porti tutti i trifogli rossi ti daró una chiave!^Si trovano tutti là in alto, sopra gli attaccapanni.", // DIALOG_MESSAGE_GETREDCLOVERS
		"Grazie per tutti i trifogli rossi, Skip! Eccoti la chiave!", 									// DIALOG_MESSAGE_GOTREDCLOVERS

		"Aloha Skip! Aiuta il pescatore a prendere qualche pesce,^saltando sui galleggianti. Se prendi 4 pesci, ti daró una chiave.", 	// DIALOG_MESSAGE_GOFISHING
		"Stai andando bene! Continua a pescare!",									// DIALOG_MESSAGE_MOREFISH
		"Sei proprio un provetto pescatore, Skip! Eccoti una chiave!",						// DIALOG_MESSAGE_THANKSFISH
		"Quelle fastidiose formiche hanno preso tutto il cibo! Ti daró una^chiave se mi riporterai 6 pezzi di cibo. Lascia cadere ciascun^pezzo sopra il cesto da picnic.", 	// DIALOG_MESSAGE_GETFOOD
		"Continua a portarmi cibo, Skip!",												// DIALOG_MESSAGE_MOREFOOD
		"Mmmmm, buono! Grazie per il cibo, Skip! Eccoti la chiave!",				// DIALOG_MESSAGE_THANKSFOOD

		"Skip, Bully Bee ha portato il tuo sacchetto dentro il favo!^Per scacciare le api bisogna affumicarle. Intanto, porta un po'^di foglie e ramoscelli sotto il favo.",				// DIALOG_MESSAGE_GETKINDLING
		"L'inizio è buono, ma non abbastanza per fare un bel faló.^Metti ancora alcune foglie e ramoscelli sotto il favo!",	// DIALOG_MESSAGE_MOREKINDLING
		"Cosí va bene, Skip! Ora ci serve un fiammifero per accendere^il fuoco!",			// DIALOG_MESSAGE_LIGHTFIRE
		"Ben fatto, Skip! Il favo è libero e ora puoi entrarci!",			// DIALOG_MESSAGE_ENTERHIVE
		"Ehi, c'è una chiave in cima alla bottiglia! Devi rompere^la bottiglia per prenderla.",						// DIALOG_MESSAGE_BOTTLEKEY

		"Skip, l'acqua della grondaia sta allagando il bidone dei^rifiuti e ci sono 9 topolini da aiutare! Salvali prima che^affoghino e ti daró una chiave.", // DIALOG_MESSAGE_MICEDROWN
		"Sei stato bravo a liberare i topolini, Skip! Ecco la chiave!",			//	DIALOG_MESSAGE_THANKSNODROWN
		"Penso che l'aereo sia il miglior modo per uscire da qui.^Peró manca qualche pezzo. Se ci rimetti l'elica e le ruote,^potrà volare nuovamente!", // DIALOG_MESSAGE_GLIDER
		"Ehi Skip, le lattine sono ancora piene. Scommetto che ce la farai^ad aprirle tutte, tirando la linguetta sul coperchio!",						// DIALOG_MESSAGE_SODACAN
	},

		/* SWEDISH */
	{
		"Hej Skip! Jag är Snigeln Sam, och jag har tappat bort mitt skal.^Om du kan hitta det åt mig så ger jag dig nyckeln till porten.",					// DIALOG_MESSAGE_NEEDSNAILSHELL
		"Tack för att du hittade mitt skal, Skip! Här är nyckeln^som jag lovade!",									// DIALOG_MESSAGE_GOTSNAILSHELL

		"Hej Skip! Om du sätter tillbaka min fågelskrämmas huvud så ger^jag dig ännu en nyckel.",					// DIALOG_MESSAGE_FINDSCARECROWHEAD
		"Bra! Du hittade fågelskrämmans huvud! Sätt nu bara tillbaka^det på honom så ger jag dig nyckeln!",	// DIALOG_MESSAGE_PUTSCARECROWHEAD
		"Tack för att du fixade min fågelskrämma, Skip! Här är din^nyckel!",											// DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD

		"Hitta min kula och använd den för att knuffa till dessa^batterier. Jag ger dig en nyckel om du kan göra det!",	// DIALOG_MESSAGE_FINDMARBLE
		"Bra jobb med att hitta min kula, Skip!  Sparka nu kulan^för att knuffa omkull alla batterierna.",			// DIALOG_MESSAGE_BOWLMARBLE
		"Jaaaaaa!! Bra jobb, Skip!  Här är ännu en nyckel.",													// DIALOG_MESSAGE_DONEBOWLING

		"Men, hej Skip!  Jag ger dig en karta om du ger mig ett ekollon.",								// DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN
		"Hejsan Skip! Jag byter den här kontrollpunkten för ett ekollon.",											// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN
		"Skip, denna stackare ser ut att sitta fast. Se om du kan lyfta^stången genom att^trycka på plocka upplockningsknappen.", 		// DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP
		"Tack, Skip! Här haru!", 																	// DIALOG_MESSAGE_CHIPMUNK_THANKS
		"Kontrollpunkt!",																						// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT

		"Skip, den röda nyckel flyter på ett löv någonstans i poolen.^Men var försiktig, kloret kan skada dig!",	// DIALOG_MESSAGE_POOLWATER

		"Om du krossar alla bären längs med stigen så ger jag dig en nyckel^vid slutet!",				// DIALOG_MESSAGE_SMASHBERRIES
		"Hörru, du missade en del bär, Skip.  Gå tillbaka och mosa alla.",								// DIALOG_MESSAGE_SQUISHMORE
		"Du ställde till en riktig röra därborta! Här är nyckeln!",												// DIALOG_MESSAGE_SQUISHDONE

		"Skip, detta rör tar dig in i huset. Jag släpper in dig om du^kan ta bort alla fästingar och loppor från min vän Fido.",	// DIALOG_MESSAGE_DOGHOUSE

		"Du hittade alla lopporna! Ta nu bort alla fästingarna!",														// DIALOG_MESSAGE_GOTFLEAS
		"Det var alla fästingarna! Ta nu kål på lopporna!!",													// DIALOG_MESSAGE_GOTTICKS
		"Bra jobb! Du har gjort Fido till en lycklig hund!",															// DIALOG_MESSAGE_HAPPY DOG
		"Kom ihåg Skip, om du får tag i alla fästingarna och lopporna från^min vän Fido, så släpper jag in dig i avloppsröret.",	// DIALOG_MESSAGE_REMEMBERDOG

		"Du kan ta dig in i huset via avloppet. Lycka till, Skip!",										// DIALOG_MESSAGE_PLUMBINGINTRO

		"Om du kan vinna över Sam i en bilbanetävling så ger jag dig^denna nyckel. Så hoppa på bilen och tävla!",		// DIALOG_MESSAGE_SLOTCAR
		"Skip, det är möss på gården som har fastnat i fällor. Om du^kan rädda minst 4 av dom så ger jag dig en nyckel till!",	// DIALOG_MESSAGE_RESCUEMICE
		"Jag ger dig en nyckel om du friar 4 möss från deras fällor!",										// DIALOG_MESSAGE_RESCUEMICE2
		"Tack för att du räddade mössen, Skip!",															// DIALOG_MESSAGE_MICESAVED

		"Du vann, Skip!  Här haru!",																		// DIALOG_MESSAGE_SLOTCARPLAYERWON
		"Åh, du kan bättre än så, Skip. Hoppa tillbaka upp och försök^igen!",								// DIALOG_MESSAGE_SLOTCARTRYAGAIN

		"Hörru, Skip! Om du kan hjälpa mig att lägga det här pusslet så^ger jag dig en nyckel.",							// DIALOG_MESSAGE_DOPUZZLE
		"Tack för att du lade pusslet åt mig, Skip! Här är en nyckel åt^dig!",								// DIALOG_MESSAGE_DONEPUZZLE

		"Om du kan förstöra alla myrstackarna talar jag om vilken^väg Bully Bi tog! Fäll en bomb ned i hålet för att förstöra en^myrstack.",	// DIALOG_MESSAGE_BOMBHILLS
		"För att fälla en bomb tryck Plocka upp-Släpp-knappen.^För att skjuta på trollsländor, tryck på Sparka-knappen.", 	// DIALOG_MESSAGE_BOMBHILLS
		"Tack för att du förstörde alla myrstackarna, Skip! Sist jag såg^Bully Bi så var han på väg mot dammen!", // DIALOG_MESSAGE_HILLSDONE

		"Hörru Skip!  Dom här bollarna verkar skrämma iväg malarna.", 												// DIALOG_MESSAGE_MOTHBALL
		"Skip, den här dörren vill inte öppna. Det verkar som om den^behöver ett par datorchips och ett batteri.", 			// DIALOG_MESSAGE_SILICONDOOR

		"Skip, jag ger dig en nyckel om du ger mig alla röda klöver!^De är där ovanför på dom där hängarna.", // DIALOG_MESSAGE_GETREDCLOVERS
		"Tack för att du gav mig alla röda klöver, Skip! Här är nyckeln!", 									// DIALOG_MESSAGE_GOTREDCLOVERS

		"Aloha Skip! Hjälp fiskaren att fånga fisk genom att hoppa på hans^fiskeflöten! Om du kan fånga 4 fiskar så ger jag dig en nyckel.", 	// DIALOG_MESSAGE_GOFISHING
		"Vad bra det går! Fortsätt att fånga fisk!",									// DIALOG_MESSAGE_MOREFISH
		"Du är värsta fiskarn, Skip! Här är en nyckel åt dig!",						// DIALOG_MESSAGE_THANKSFISH
		"De där odrägliga myrorna har stulit all mat! Jag ger dig en nickel^om du tar tillbaka 6 bitar mat åt mig. Släpp dom^varsomhelst på picknickkorgen.", 	// DIALOG_MESSAGE_GETFOOD
		"Fortsätt att ge mig mat, Skip!",												// DIALOG_MESSAGE_MOREFOOD
		"Mmmmm, gott! Tack för maten, Skip!  Här är nyckeln!",				// DIALOG_MESSAGE_THANKSFOOD

		"Skip, Bully Bi tog din väska in till bikupan! Det enda sättet jag vet^som får bin att fly är att röka ut dom, så samla ihop^löv och grenar under kupan.",				// DIALOG_MESSAGE_GETKINDLING
		"Det är en bra början, men det är inte nog med tändmaterial för att^göra en stor brasa. Samla ihop mer löv och grenar under kupan!",	// DIALOG_MESSAGE_MOREKINDLING
		"Det ser bra ut, Skip! Nu behöver vi bara en gnista för att tända^brasan!",			// DIALOG_MESSAGE_LIGHTFIRE
		"Bra jobb, Skip! Det verkar nu säkert att gå in i kupan!",			// DIALOG_MESSAGE_ENTERHIVE
		"Hörru, det är en nyckel uppe på flaskan. Du måste ha sönder den^för att få tag i den.",						// DIALOG_MESSAGE_BOTTLEKEY

		"Skip, vatten från takrännan flödar över soptunnan, och det är^9 möss däri som behöver din hjälp! Rädda dom innan de^drunknar och jag ger dig en nyckel.", // DIALOG_MESSAGE_MICEDROWN
		"Bra jobb med att rädda dom där mössen, Skip! Här är nyckeln!",			//	DIALOG_MESSAGE_THANKSNODROWN
		"Jag tror att det bästa sättet för dig att ta dig ut härifrån är genom^att flyga det här planet, men idet saknar en del delar.^Jag slår vad att om du sätter tillbaka propellern och hjulen så flyger det!", // DIALOG_MESSAGE_GLIDER
		"Hörru Skip, dom här läskburkarna är fortfarande fulla. Jag slår^vad om att du kan öppna fliken på dom!",						// DIALOG_MESSAGE_SODACAN
	},

		/* DUTCH */
	{
		"Hoi Skip! Ik ben Sam de Slak, en het lijkt erop dat ik mijn huis^kwijt ben. Als je het voor me kunt vinden geef ik je de^sleutel voor de poort.",
		"Bedankt voor het vinden van mijn huis, Skip! Hier is de sleutel^die ik je beloofd heb!",


		"Hoi Skip! Als je het hoofd van mijn vogelverschrikker terug kunt^zetten geef ik je nog een sleutel.",
		"Geweldig! Je hebt het hoofd van de vogelverschrikker gevonden!^Zet het nu maar bij hem op en dan geef ik je de sleutel!",
		"Bedankt voor het repareren van mijn vogelverschrikker, Skip!^Hier is je sleutel!",


		"Zoek mijn knikker en gebruik hem om deze batterijen om te^gooien. Ik geef je een sleutel als het je lukt!",
		"Goed gedaan Skip, je hebt mijn knikker gevonden! Geef hem een^trap om alle batterijen om te gooien.",
		"Jippieee!! Goed gedaan, Skip! Hier heb je nog een sleutel.",



		"Hallo daar, Skip! Ik geef je een plattegrond als je mij^een eikel brengt.",
		"Hoi Skip! Ik ruil dit checkpoint voor een eikel.",

		"Skip, volgens mij zit dit beestje vast. Probeer de val open^te maken door op de 'Oppak' knop te drukken.",
		"Bedankt, Skip! Alsjeblieft!",


		"Checkpoint!",

		"Skip, de rode sleutel drijft op een blaadje ergens in het zwembad.^Maar kijk uit, het chloor in het water doet pijn!",

		"Als je alle bessen langs het parcours kunt pletten geef ik je een^sleutel aan het eind!",
		"Hé, je hebt een paar bessen gemist Skip. Ga terug en plet ze^allemaal.",
		"Je hebt er nogal een smeerboel van gemaakt! Hier is de sleutel!",


		"Skip, door deze pijp kom je in het huis. Ik laat je binnen als^je alle teken en vlooien van mijn vriend Fido kunt verwijderen.",

		"Je hebt alle vlooien! Ga nu achter de teken aan!",

		"Dat waren de teken! Maak nu een eind aan de vlooien!",

		"Goed zo! Nu is Fido weer een blije hond!",

		"Onthoud Skip, als je alle teken en vlooien van mijn vriend^Fido kunt verwijderen laat ik je in de afvoerpijp.",

		"Het riool voert je in het huis. Succes Skip!",


		"Als je Sam kunt verslaan in de autorace geef ik je deze^sleutel. Spring dus in de auto en ga racen!",
		"Skip, er zijn muizen in de tuin die gevangen zijn in vallen.^Als je er ten minste vier kunt bevrijden geef ik je nog een sleutel!",
		"Ik geef je een sleutel als je vier muizen kunt bevrijden uit^hun vallen!",

		"Bedankt voor het redden van die muizen, Skip!",



		"Je hebt gewonnen, Skip! Alsjeblieft!",


		"Tjonge Skip, dat kun je toch wel beter? Spring er nog^eens in en probeer het opnieuw!",


		"Hé Skip! Als je me helpt met deze puzzel geef ik je een^sleutel.",
		"Bedankt voor het afmaken van de puzzel Skip! Hier is^een sleutel voor je!",


		"Als je alle mierenhopen kunt vernietigen zal ik je vertellen^welke kant Pest Wesp heen is gegaan! Om een mierenhoop te^vernietigen gooi je een bom in het gat.",
		"Om een bom te gooien druk je op de 'Oppak' knop. Om op^Libellen te schieten druk je op de 'Schop' knop.",
		"Bedankt voor het vernietigen van alle mierenhopen Skip!^De laatste keer dat ik Pest Wesp zag ging hij naar de vijver!",

		"Hé Skip! Het lijkt erop dat de Motten bang zijn voor^deze ballen.",

		"Skip, deze deur gaat niet open. Het lijkt erop dat hij^een paar nieuwe computer chips en een batterij nodig heeft.",

		"Skip, ik geef je een sleutel als je me alle rode klavers^brengt! Ze liggen allemaal daarboven op die kleerhangers.",
		"Bedankt voor het halen van alle rode klavers Skip! Hier is^de sleutel!",


		"Aloha Skip! Help de visser wat vis vangen door op zijn^dobbers te springen! Als je vier vissen kunt vangen geef ik^je een sleutel.",
		"Het gaat geweldig! Blijf vissen vangen!",

		"Je bent een prima visser Skip! Hier is een sleutel voor je!",

		"Die vervelende mieren hebben al het eten gestolen! Ik^geef je een sleutel als je zes porties eten voor me terug brengt.^Leg ze maar op de picknickmand.",
		"Blijf me eten brengen Skip!",

		"Mmmmm, lekker! Bedankt voor het eten Skip! Hier is de sleutel!",


		"Skip, Pest Wesp heeft je tas meegenomen in de Bijenkorf!^De enige manier die ik ken om de bijen te verdrijven is door ze uit^te roken, dus ga blaadjes en takjes verzamelen onder de bijenkorf.",
		"Dat is een goed begin, maar het is nog niet genoeg^aanmaakhout om een groot vuur te maken. Verzamel meer^blaadjes en takjes onder de bijenkorf!",
		"Dat ziet er goed uit Skip! Nu hebben we alleen nog een vonkje^nodig om het vuur te ontsteken!",

		"Goed gedaan Skip! De bijenkorf ziet er nu veilig genoeg uit om^naar binnen te gaan!",
		"Hé, er ligt een sleutel boven op de fles. Je zult hem moeten^breken om hem te pakken.",

		"Skip, de vuilnis emmer loopt vol met water uit de goot en er^zijn negen muizen die je hulp nodig hebben! Als je ze redt voor^ze verdrinken geef ik je een sleutel.",
		"Goed gedaan Skip, je hebt alle muizen bevrijd! Hier is de^sleutel!",
		"Ik denk dat je het beste hier weg kunt komen door met dat^vliegtuig weg te vliegen, maar er ontbreken een paar onderdelen.^Ik wed dat hij vliegt als je de propeller en de wielen terug zet!",
		"Hé Skip, deze blikjes fris zijn nog steeds vol. Ik wed dat^je de lipjes open kunt trekken!",
	}
};


static const int gMessageIcon[MAX_DIALOG_MESSAGES][2] =					// Sprite Group, Sprite #
{
	SPRITE_GROUP_LEVELSPECIFIC, GARDEN_SObjType_SnailShellIcon,			// DIALOG_MESSAGE_NEEDSNAILSHELL
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_RedKeyIcon,					// DIALOG_MESSAGE_GOTSNAILSHELL

	SPRITE_GROUP_LEVELSPECIFIC,GARDEN_SObjType_ScarecrowHeadIcon,		// DIALOG_MESSAGE_FINDSCARECROWHEAD
	SPRITE_GROUP_LEVELSPECIFIC,GARDEN_SObjType_ScarecrowHeadIcon,		// DIALOG_MESSAGE_PUTSCARECROWHEAD
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_GreenKeyIcon,					// DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD

	SPRITE_GROUP_LEVELSPECIFIC,PLAYROOM_SObjType_MarbleIcon,			// DIALOG_MESSAGE_FINDMARBLE
	SPRITE_GROUP_LEVELSPECIFIC,PLAYROOM_SObjType_MarbleIcon,			// DIALOG_MESSAGE_BOWLMARBLE
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_BlueKeyIcon,					// DIALOG_MESSAGE_DONEBOWLING

	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_AcornIcon,						// DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_AcornIcon,						// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN
	SPRITE_GROUP_LEVELSPECIFIC,GARDEN_SObjType_Mouse,					// DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP
	-1,-1,																// DIALOG_MESSAGE_CHIPMUNK_THANKS
	-1,-1,																// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT

	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_RedKeyIcon,					// DIALOG_MESSAGE_POOLWATER

	SPRITE_GROUP_LEVELSPECIFIC,SIDEWALK_SObjType_SquishBerry,			// DIALOG_MESSAGE_SMASHBERRIES
	SPRITE_GROUP_LEVELSPECIFIC,SIDEWALK_SObjType_SquishBerry,			// DIALOG_MESSAGE_SQUISHMORE
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_GreenKeyIcon,					// DIALOG_MESSAGE_SQUISHDONE

	-1,-1,																// DIALOG_MESSAGE_DOGHOUSE

	SPRITE_GROUP_LEVELSPECIFIC, FIDO_SObjType_Tick,						// DIALOG_MESSAGE_GOTFLEAS
	SPRITE_GROUP_LEVELSPECIFIC, FIDO_SObjType_Flea,						// DIALOG_MESSAGE_GOTTICKS
	-1, -1,																// DIALOG_MESSAGE_HAPPYDOG
	SPRITE_GROUP_LEVELSPECIFIC, FIDO_SObjType_Flea,						// DIALOG_MESSAGE_REMEMBERDOG

	-1,-1,																// DIALOG_MESSAGE_PLUMBINGINTRO

	-1,-1,																// DIALOG_MESSAGE_SLOTCAR
	SPRITE_GROUP_LEVELSPECIFIC,GARDEN_SObjType_Mouse,					// DIALOG_MESSAGE_RESCUEMICE
	SPRITE_GROUP_LEVELSPECIFIC,GARDEN_SObjType_Mouse,					// DIALOG_MESSAGE_RESCUEMICE2
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_RedKeyIcon,					// DIALOG_MESSAGE_MICESAVED

	-1,-1,																// DIALOG_MESSAGE_SLOTCARPLAYERWON
	-1,-1,																// DIALOG_MESSAGE_SLOTCARTRYAGAIN

	-1,-1,																// DIALOG_MESSAGE_DOPUZZLE
	-1,-1,																// DIALOG_MESSAGE_DONEPUZZLE

	SPRITE_GROUP_LEVELSPECIFIC,BALSA_SObjType_AntHillIcon, 				// DIALOG_MESSAGE_BOMBHILLS
	SPRITE_GROUP_LEVELSPECIFIC,BALSA_SObjType_AntHillIcon, 				// DIALOG_MESSAGE_BOMBHILLS2
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_BeeIcon,						// DIALOG_MESSAGE_HILLSDONE

	SPRITE_GROUP_LEVELSPECIFIC,CLOSET_SObjType_MothBallIcon,			// DIALOG_MESSAGE_MOTHBALL
	SPRITE_GROUP_LEVELSPECIFIC,CLOSET_SObjType_ChipIcon,				// DIALOG_MESSAGE_SILICONDOOR
	SPRITE_GROUP_LEVELSPECIFIC,CLOSET_SObjType_RedClover,				// DIALOG_MESSAGE_GETREDCLOVERS
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_RedKeyIcon,					// DIALOG_MESSAGE_GOTREDCLOVERS

	SPRITE_GROUP_LEVELSPECIFIC,PARK_SObjType_BobberIcon,				// DIALOG_MESSAGE_GOFISHING
	SPRITE_GROUP_LEVELSPECIFIC,PARK_SObjType_BobberIcon,				// DIALOG_MESSAGE_MOREFISH
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_RedKeyIcon,					// DIALOG_MESSAGE_THANKSFISH
	SPRITE_GROUP_LEVELSPECIFIC,PARK_SObjType_CheeseIcon,				// DIALOG_MESSAGE_GETFOOD
	SPRITE_GROUP_LEVELSPECIFIC,PARK_SObjType_CheeseIcon,				// DIALOG_MESSAGE_MOREFOOD
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_BlueKeyIcon,					// DIALOG_MESSAGE_THANKSFOOD

	SPRITE_GROUP_LEVELSPECIFIC,PARK_SObjType_KindlingIcon,				// DIALOG_MESSAGE_GETKINDLING
	SPRITE_GROUP_LEVELSPECIFIC,PARK_SObjType_KindlingIcon,				// DIALOG_MESSAGE_MOREKINDLING
	SPRITE_GROUP_LEVELSPECIFIC,PARK_SObjType_FireIcon,					// DIALOG_MESSAGE_LIGHTFIRE
	SPRITE_GROUP_LEVELSPECIFIC,PARK_SObjType_HiveIcon,					// DIALOG_MESSAGE_ENTERHIVE
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_GreenKeyIcon,					// DIALOG_MESSAGE_BOTTLEKEY

	SPRITE_GROUP_LEVELSPECIFIC,GARBAGE_SObjType_Mouse,					// DIALOG_MESSAGE_MICEDROWN
	SPRITE_GROUP_DIALOG,DIALOG_SObjTypes_RedKeyIcon,					// DIALOG_MESSAGE_THANKSNODROWN
	SPRITE_GROUP_LEVELSPECIFIC,GARBAGE_SObjType_PropIcon,				// DIALOG_MESSAGE_GLIDER
	SPRITE_GROUP_LEVELSPECIFIC,GARBAGE_SObjType_CanIcon,				// DIALOG_MESSAGE_SODACAN
};


static const int gMessageSound[MAX_DIALOG_MESSAGES] =
{
	EFFECT_SAM_FINDSHELL,		// DIALOG_MESSAGE_NEEDSNAILSHELL
	EFFECT_SAM_GOTSHELL,		// DIALOG_MESSAGE_GOTSNAILSHELL

	EFFECT_SAM_FINDHEAD,		// DIALOG_MESSAGE_FINDSCARECROWHEAD
	EFFECT_SAM_PUTHEADON,		// DIALOG_MESSAGE_PUTSCARECROWHEAD
	EFFECT_SAM_FIXEDSCARECROW,	// DIALOG_MESSAGE_ATTACHEDSCARECROWHEAD

	EFFECT_SAM_FINDMARBLE,		// DIALOG_MESSAGE_FINDMARBLE
	EFFECT_SAM_KICKMARBLE,		// DIALOG_MESSAGE_BOWLMARBLE
	EFFECT_SAM_PINSDOWN,		// DIALOG_MESSAGE_DONEBOWLING

	EFFECT_CHIP_MAP4ACORN,		// DIALOG_MESSAGE_CHIPMUNK_MAP4ACORN
	EFFECT_CHIP_CHECKPOINT,		// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT4ACORN
	EFFECT_CHIP_STUCKMOUSE,		// DIALOG_MESSAGE_CHIPMUNK_MOUSETRAP
	EFFECT_CHIP_THANKS,			// DIALOG_MESSAGE_CHIPMUNK_THANKS
	EFFECT_CHIP_CHECKPOINTDONE,	// DIALOG_MESSAGE_CHIPMUNK_CHECKPOINT

	EFFECT_SAM_POOLKEY,			// DIALOG_MESSAGE_POOLWATER

	EFFECT_SAM_SQUASHBERRIES,	// DIALOG_MESSAGE_SMASHBERRIES
	EFFECT_SAM_SQUISHMORE,		// DIALOG_MESSAGE_SQUISHMORE
	EFFECT_SAM_SQUISHDONE,		// DIALOG_MESSAGE_SQUISHDONE

	EFFECT_SAM_FIDO,			// DIALOG_MESSAGE_DOGHOUSE

	EFFECT_GOTFLEAS,			// DIALOG_MESSAGE_GOTFLEAS
	EFFECT_GOTTICKS,			// DIALOG_MESSAGE_GOTTICKS
	EFFECT_HAPPYDOG,			// DIALOG_MESSAGE_HAPPYDOG
	EFFECT_REMEMBERDOG,			// DIALOG_MESSAGE_REMEMBERDOG

	EFFECT_PLUMBINGINTRO,		// DIALOG_MESSAGE_PLUMBINGINTRO

	EFFECT_CHIP_DORACE,			// DIALOG_MESSAGE_SLOTCAR
	EFFECT_SAM_FREEMICE,		// DIALOG_MESSAGE_RESCUEMICE
	EFFECT_SAM_FREEMICE2,		// DIALOG_MESSAGE_RESCUEMICE2
	EFFECT_SAM_GOTMICE,			// DIALOG_MESSAGE_MICESAVED

	EFFECT_CHIP_YOUWON,			// DIALOG_MESSAGE_SLOTCARPLAYERWON
	EFFECT_CHIP_LOSTRACE,		// DIALOG_MESSAGE_SLOTCARTRYAGAIN

	EFFECT_SAM_DOPUZZLE,		// DIALOG_MESSAGE_DOPUZZLE
	EFFECT_SAM_PUZZLEDONE,		// DIALOG_MESSAGE_DONEPUZZLE

	EFFECT_SAM_DESTROYHILLS,	// DIALOG_MESSAGE_BOMBHILLS
	EFFECT_SAM_HOWTOBOMB,		// DIALOG_MESSAGE_BOMBHILLS2
	EFFECT_SAM_HILLSDESTROYED,	// DIALOG_MESSAGE_HILLSDONE

	EFFECT_SAM_MOTHBALLS,		// DIALOG_MESSAGE_MOTHBALL
	EFFECT_SAM_COMPUTERDOOR,	// DIALOG_MESSAGE_SILICONDOOR
	EFFECT_SAM_GETREDCLOVERS,	// DIALOG_MESSAGE_GETREDCLOVERS
	EFFECT_SAM_GOTREDCLOVERS,	// DIALOG_MESSAGE_GOTREDCLOVERS

	EFFECT_SAM_CATCHFISH,		// DIALOG_MESSAGE_GOFISHING
	EFFECT_SAM_KEEPFISHING,		// DIALOG_MESSAGE_MOREFISH
	EFFECT_SAM_ANGLER,			// DIALOG_MESSAGE_THANKSFISH
	EFFECT_SAM_GETFOOD,			// DIALOG_MESSAGE_GETFOOD
	EFFECT_SAM_MOREFOOD,		// DIALOG_MESSAGE_MOREFOOD
	EFFECT_SAM_GOTFOOD,			// DIALOG_MESSAGE_THANKSFOOD

	EFFECT_SAM_GETKINDLING,		// DIALOG_MESSAGE_GETKINDLING
	EFFECT_SAM_MOREKINDLING,	// DIALOG_MESSAGE_MOREKINDLING
	EFFECT_SAM_SPARK,			// DIALOG_MESSAGE_LIGHTFIRE
	EFFECT_SAM_ENTERHIVE,		// DIALOG_MESSAGE_ENTERHIVE
	EFFECT_SAM_BOTTLEKEY,		// DIALOG_MESSAGE_BOTTLEKEY

	EFFECT_SAM_GUTTERWATER,		// DIALOG_MESSAGE_MICEDROWN
	EFFECT_SAM_FREED,			// DIALOG_MESSAGE_THANKSNODROWN
	EFFECT_SAM_GLIDER,			// DIALOG_MESSAGE_GLIDER
	EFFECT_SAM_SODA,			// DIALOG_MESSAGE_SODACAN
};


static	float	gMessageReplayVoiceDelay[MAX_DIALOG_MESSAGES];


static	int			gCurrentDialogMessageNum;
static	char const *gCurrentDialogString;
static	int			gCurrentDialogIconGroup,gCurrentDialogIconFrame;
static	int			gNumLinesInCurrentDialog = 0;						// when 0, indicates no message active
short				gDialogSoundChannel;
int					gDialogSoundEffect;
static	OGLPoint3D	gDialogSoundWhere;

static	int		gNextMessageNum;
static	int		gNextMessagePriority,gCurrentDialogMessagePriority;
static	float	gNextMessageDuration, gMessageDuration;
static	OGLPoint3D	gNextMessageWhere;


/****************** INIT DIALOG MANAGER *************************/

void InitDialogManager(void)
{
FSSpec	spec;
int		i;

	gNumLinesInCurrentDialog = 0;
	gDialogAlpha			= 0;
	gNextMessageNum = gCurrentDialogMessageNum = -1;
	gNextMessagePriority = 100;
	gNextMessageDuration = 0;
	gDialogSoundChannel = -1;

	gCurrentDialogIconGroup = gCurrentDialogIconFrame = -1;

	for (i = 0; i < MAX_DIALOG_MESSAGES; i++)
		gMessageReplayVoiceDelay[i] = 0;

			/* LOAD THE SPRITES FOR THE DIALOGS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, "\p:Sprites:Dialog.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_DIALOG, gGameViewInfoPtr);
}


/**************************** DO DIALOG MESSAGE *******************************/

void DoDialogMessage(int messNum, int priority, float duration, OGLPoint3D *fromWhere)
{
	if (messNum == gCurrentDialogMessageNum)						// if same message, then just reset timer
	{
		if (gDialogMode == DIALOG_MODE_DISAPPEAR)					// if was disappearing, then reverse
			gDialogMode = DIALOG_MODE_APPEAR;

		gMessageDuration = duration;
		return;
	}

			/* SEE IF THIS MESSAGE HAS HIGHER PRIORITY */

	if (gNumLinesInCurrentDialog == 0)								// if no dialog currently, then just do it
	{
		CreateDialogMessage(messNum, priority, duration, fromWhere);
		return;
	}

	if (priority < gCurrentDialogMessagePriority)					// if higher priority then make current message go away
		gDialogMode = DIALOG_MODE_DISAPPEAR;


				/* MAKE THIS THE NEXT MESSAGE */

	gNextMessageNum = messNum;
	gNextMessagePriority = priority;
	gNextMessageDuration = duration;

	if (fromWhere == nil)
		gNextMessageWhere.x = gNextMessageWhere.y = gNextMessageWhere.z = 0;
	else
		gNextMessageWhere = *fromWhere;

}


/********************* CREATE DIALOG MESSAGE **************************/

static void CreateDialogMessage(int messNum, int priority, float duration, OGLPoint3D *fromWhere)
{
int		i;
int		effect;
int		language = gGamePrefs.language;

	gCurrentDialogIconGroup = gMessageIcon[messNum][0];								// get sprite group
	gCurrentDialogIconFrame = gMessageIcon[messNum][1];								// get sprite frame

	gCurrentDialogString = gTotalDialogString[language][messNum];					// get ptr to string

				/* SEE HOW MANY LINES OF TEXT */

	i = 0;
	gNumLinesInCurrentDialog = 1;
	while (gCurrentDialogString[i] != 0x00)
	{
		if (gCurrentDialogString[i] == '^')						// look for ^ character which denotes a next-line
			gNumLinesInCurrentDialog++;
		i++;
	}

	gCurrentDialogMessageNum = messNum;
	gCurrentDialogMessagePriority = priority;
	gMessageDuration = duration;
	gDialogMode = DIALOG_MODE_APPEAR;

	gNextMessageNum = -1;										// no message waiting

			/* PLAY AUDIO */

	if (gMessageReplayVoiceDelay[messNum] == 0.0f)				// see if there's still a delay
	{
		effect = gMessageSound[messNum];
		if (effect != -1)
		{
			if (gDialogSoundChannel != -1)						// stop any existing audio
				StopAChannelIfEffectNum(&gDialogSoundChannel, gDialogSoundEffect);

			if (fromWhere)
			{
				gDialogSoundChannel = PlayEffect_Parms3D(effect, fromWhere, NORMAL_CHANNEL_RATE, 1.2);
				gDialogSoundWhere = *fromWhere;
			}
			else
			{
				gDialogSoundChannel = PlayEffect(effect);
				gDialogSoundWhere.x = gDialogSoundWhere.y = gDialogSoundWhere.z = 0.0;
			}

			gDialogSoundEffect = effect;

			gMessageReplayVoiceDelay[messNum] = 20.0f;			// set delay before can play this voice again
		}
	}
}


/************************ DRAW DIALOG MESSAGE ********************/

void DrawDialogMessage(const OGLSetupOutputType *setupInfo)
{
int		i;
float	x,y,leftX;
AGLContext agl_ctx = setupInfo->drawContext;

			/* UPDATE ANY CURRENT VOICE */

	if (gDialogSoundChannel != -1)
	{
		if ((gDialogSoundWhere.x != 0.0f) || (gDialogSoundWhere.y != 0.0f) || (gDialogSoundWhere.z != 0.0f))	// only update if not 0,0,0
			Update3DSoundChannel(gDialogSoundEffect, &gDialogSoundChannel, &gDialogSoundWhere);
	}

	for (i = 0; i < MAX_DIALOG_MESSAGES; i++)					// update delay timers while we're here
	{
		if (gMessageReplayVoiceDelay[i] > 0.0f)
		{
			gMessageReplayVoiceDelay[i] -= gFramesPerSecondFrac;
			if (gMessageReplayVoiceDelay[i] < 0.0f)
				gMessageReplayVoiceDelay[i] = 0.0f;
		}
	}

	if (gNumLinesInCurrentDialog == 0)
		return;

	SetColor4f(1,1,1,1);

		/***********************************/
		/* MOVE FRAME INTO POSTIONN & DRAW */
		/***********************************/

	switch(gDialogMode)
	{
		case	DIALOG_MODE_APPEAR:
				gDialogAlpha += gFramesPerSecondFrac * 2.0f;
				if (gDialogAlpha >= 1.0f)
				{
					gDialogAlpha = 1.0;
					gDialogMode = DIALOG_MODE_STAY;
				}
				break;

		case	DIALOG_MODE_DISAPPEAR:
				gDialogAlpha -= gFramesPerSecondFrac * 2.0f;
				if (gDialogAlpha <= 0.0f)
				{
					gDialogAlpha = 0.0f;
					gDialogMode = DIALOG_MODE_NONE;
					gCurrentDialogMessagePriority = 100;
					gCurrentDialogMessageNum = -1;
					if (gNextMessageNum != -1)
					{
						if ((gNextMessageWhere.x == 0.0f) && (gNextMessageWhere.y == 0.0f) && (gNextMessageWhere.z == 0.0f))		// see if has coord or not
							CreateDialogMessage(gNextMessageNum, gNextMessagePriority, gNextMessageDuration, nil);
						else
							CreateDialogMessage(gNextMessageNum, gNextMessagePriority, gNextMessageDuration, &gNextMessageWhere);
					}
				}
				break;

		case	DIALOG_MODE_STAY:
				gMessageDuration -= gFramesPerSecondFrac;
				if (gMessageDuration <= 0.0f)
					gDialogMode = DIALOG_MODE_DISAPPEAR;
				break;

		case	DIALOG_MODE_NONE:
				break;

	}


			/* SET FADE IN/OUT BASED ON DISTANCE FROM STOP POSITION */

	gGlobalTransparency = gDialogAlpha;


			/* DRAW FRAME FIRST */

	x = (640-DIALOG_FRAME_WIDTH)/2 + 5.0f;
	y = 410.0f;
	DrawInfobarSprite2(x, y, DIALOG_FRAME_WIDTH, SPRITE_GROUP_DIALOG, DIALOG_SObjTypes_Frame, setupInfo);


			/* DRAW ICON */

	if (gCurrentDialogIconGroup != -1)						// see if has icon
	{
		DrawInfobarSprite2(x+6, y+6, DIALOG_ICON_WIDTH, gCurrentDialogIconGroup, gCurrentDialogIconFrame, setupInfo);
	}


			/*************/
			/* DRAW TEXT */
			/*************/

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow

	gGlobalColorFilter.r = 1.0f;
	gGlobalColorFilter.g = 1.0f;
	gGlobalColorFilter.b = .8f;

	leftX = x += 80.0f;

			/* SET VERTICAL CENTERING */

	y += 10;

	if (gNumLinesInCurrentDialog == 1)						// center based on # of lines of text
		y += LETTER_SIZE * 1.3f;
	else
	if (gNumLinesInCurrentDialog == 2)
		y += LETTER_SIZE * 1.3f * .5f;


			/* DRAW EACH CHAR */

	i = 0;
	while (gCurrentDialogString[i] != 0x00)
	{
			/* NEXT LINE */

		if (gCurrentDialogString[i] == '^')						// look for ^ character which denotes a next-line
		{
			y += LETTER_SIZE * 1.3f;
			x = leftX;
		}

			/* DRAW LETTER */
		else
		{
			char	c = gCurrentDialogString[i];
			int	texNum = CharToSprite(c);
			if (texNum != -1)
				DrawInfobarSprite2(x, y, LETTER_SIZE * 1.8f, SPRITE_GROUP_DIALOG, texNum, setupInfo);

			x += GetCharSpacing(c, LETTER_SPACING);
		}
		i++;
	}

	gGlobalColorFilter.r = 1.0f;
	gGlobalColorFilter.g = 1.0f;
	gGlobalColorFilter.b = 1.0f;

	gGlobalTransparency = 1.0f;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}




#pragma mark -


/***************** CHAR TO SPRITE **********************/

int CharToSprite(char c)
{

	if ((c >= 'a') && (c <= 'z'))
	{
		return(DIALOG_SObjType_a + (c - 'a'));
	}
	else
	if ((c >= 'A') && (c <= 'Z'))
	{
		return(DIALOG_SObjType_A + (c - 'A'));
	}
	else
	if ((c >= '0') && (c <= '9'))
	{
		return(DIALOG_SObjType_0 + (c - '0'));
	}
	else
	{
		short	s;
		switch(c)
		{
			case	'.':
					s = DIALOG_SObjType_Period;
					break;

			case	',':
					s = DIALOG_SObjType_Comma;
					break;

			case	'-':
					s = DIALOG_SObjType_Dash;
					break;

			case	'?':
					s = DIALOG_SObjType_QuestionMark;
					break;

			case	'!':
					s = DIALOG_SObjType_ExclamationMark;
					break;

			case	'¡':
					s = DIALOG_SObjType_ExclamationMark2;
					break;

			case	'Ü':
					s = DIALOG_SObjType_UU;
					break;

			case	'ü':
					s = DIALOG_SObjType_uu;
					break;

			case	'ú':
					s = DIALOG_SObjType_ua;
					break;

			case	'Ö':
					s = DIALOG_SObjType_OO;
					break;

			case	'ö':
					s = DIALOG_SObjType_oo;
					break;

			case	'Ä':
					s = DIALOG_SObjType_AA;
					break;

			case	'Å':
					s = DIALOG_SObjType_AO;
					break;

			case	'â':
					s = DIALOG_SObjType_av;
					break;

			case	'ä':
					s = DIALOG_SObjType_au;
					break;

			case	'á':
					s = DIALOG_SObjType_aa;
					break;

			case	'Ñ':
					s = DIALOG_SObjType_NN;
					break;

			case	'ñ':
					s = DIALOG_SObjType_nn;
					break;

			case	'É':
					s = DIALOG_SObjType_EE;
					break;

			case	'é':
					s = DIALOG_SObjType_ee;
					break;

			case	'è':
					s = DIALOG_SObjType_ee;
					break;

			case	'ê':
					s = DIALOG_SObjType_ev;
					break;

			case	'È':
					s = DIALOG_SObjType_EE;
					break;

			case	'Ê':
					s = DIALOG_SObjType_E;
					break;

			case	'À':
					s = DIALOG_SObjType_Ax;
					break;

			case	'à':
					s = DIALOG_SObjType_ax;
					break;

			case	'å':
					s = DIALOG_SObjType_ao;
					break;


			case	'Ô':
					s = DIALOG_SObjType_Ox;
					break;

			case	'Ó':
					s = DIALOG_SObjType_Oa;
					break;

			case	'ó':
					s = DIALOG_SObjType_oa;
					break;

			case	'ß':
					s = DIALOG_SObjType_beta;
					break;

			case	'í':
					s = DIALOG_SObjType_ia;
					break;

			case	'Ç':
					s = DIALOG_SObjType_C;
					break;

			case	'ç':
					s = DIALOG_SObjType_c;
					break;

			case	CHAR_APOSTROPHE:
					s = DIALOG_SObjType_Apostrophe;
					break;


			default:
					s = -1;

		}
	return(s);
	}

}


/******************** GET CHAR SPACING *************************/

float GetCharSpacing(char c, float spacingScale)
{
float	s;

	switch(c)
	{
		case	'j':
		case	'9':
		case	'ä':
				s = .6;
				break;

		case	' ':
		case	'f':
		case	't':
		case	'r':
				s = .5f;
				break;

		case	'I':
		case	'i':
		case	'í':
		case	'l':
		case	'.':
		case	',':
		case	'!':
		case	'¡':
				s = .4;
				break;

		case	CHAR_APOSTROPHE:
				s = .3;
				break;

		case	'a':
		case	'á':
		case	'à':
		case	'â':
		case	'å':
		case	'b':
		case	'c':
		case	'ç':
		case	'd':
		case	'e':
		case	'é':
		case	'ê':
		case	'g':
		case	'h':
		case	'k':
		case	'n':
		case	'ñ':
		case	'o':
		case	'ö':
		case	'ó':
		case	'p':
		case	'q':
		case	's':
		case	'u':
		case	'ü':
		case	'ú':
		case	'v':
		case	'x':
		case	'y':
		case	'z':
		case	'J':
		case	'S':
		case	'T':
		case	'Y':
		case	'0':
		case	'1':
		case	'2':
		case	'3':
		case	'4':
		case	'5':
		case	'6':
		case	'7':
		case	'8':
		case	'L':
				s = .7f;
				break;

		case	'N':
		case	'P':
		case	'B':
		case	'C':
		case	'Ç':
		case	'V':
		case	'F':
		case	'H':
		case	'ß':
				s = .8f;
				break;


		case	'M':
		case	'm':
				s = 1.0f;
				break;

		default:
				s = .9f;
	}





	return(spacingScale * s);


}



