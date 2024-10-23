//g++ `pkg-config --libs --cflags playerc++ opencv` -lkatana -lespeak sonarobstacleavoidKATANA.cc -o evadir

#include "katana.h"

int main()
{
  Katana robot("amigobot07");

  double min_front_dist = 0.550;                //0.5
  double really_min_front_dist = 0.350;         //0.3
  char avoid;
  double sp[8];

  double newspeed = 0.0, newturnrate = 0.0;

  for(;;){
	robot.getRobotSensor();

        for(int i=0; i<8; i++){
            sp[i] = robot.getSonar(i);
            //cout << sp[i] << "\t";
        }
        //cout << "\n";

        avoid = 0;
        newspeed = 0.100;       // 0.2000

        if (avoid == 0){

          if((sp[2] < really_min_front_dist) ||
             (sp[3] < really_min_front_dist) ||
             (sp[4] < really_min_front_dist) ||
             (sp[5] < really_min_front_dist)){
                avoid = 50;
                newspeed = -0.0500;	//-0.100
             }

          else if((sp[2] < min_front_dist) ||
                  (sp[3] < min_front_dist) ||
                  (sp[4] < min_front_dist) ||
                  (sp[5] < min_front_dist)){
                        newspeed = 0;
                        avoid = 50;
                  }
      }//if 1

      if(avoid > 0){
        if((sp[0] + sp[1]) <
           (sp[6] + sp[7]))
          newturnrate = dtor(-15);	//30
        else
          newturnrate = dtor(15);
        avoid--;
      }
      else
	newturnrate = 0;

      // write commands to robot
      cout << 2*newspeed << "\t " << newturnrate << endl;
      robot.SetSpeed(2 * newspeed, newturnrate);
    }

return 0;
}


