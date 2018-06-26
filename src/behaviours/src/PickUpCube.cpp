#include "PickUpCube.hpp"
#include "SwarmieInterface.hpp"

PickUpCube::PickUpCube(const SwarmieSensors* sensors) :
   Behavior(sensors),
   _state(NotHolding),
   _allowReset(true),
   _distanceToTarget(OUT_OF_RANGE)
{
   _pickupTimer = _nh.createTimer(ros::Duration(0), &PickUpCube::PickupTimeout, this, true);
   _pickupTimer.stop();
   _checkTimer  = _nh.createTimer(ros::Duration(4), &PickUpCube::CheckTimeout, this, true);
   _checkTimer.stop();
   _recheckTimer = _nh.createTimer(ros::Duration(30), &PickUpCube::RecheckHandler, this);
   _recheckTimer.stop();
}

void PickUpCube::SetRecheckInterval(double t)
{
   _recheckTimer.stop();
   _recheckTimer.setPeriod(ros::Duration(t), true);
   _recheckTimer.start();
}

void PickUpCube::CheckTimeout(const ros::TimerEvent& event)
{
   std::cout << "in check timeout" << std::endl;
   // only transition if we meant to check.
   if(_state != Checking && _state != Rechecking) return;
   
   if(_sensors->GetCenterSonar() < 0.12 || _distanceToTarget < 0.14)
   {
      _state = Holding;
      _recheckTimer.start();
   }
   else
   {
      _state = NotHolding;
   }
}

void PickUpCube::RecheckHandler(const ros::TimerEvent& event)
{
   if(_state == Holding)
   {
      _checkTimer.stop();
      _checkTimer.setPeriod(ros::Duration(3), true);
      std::cout << "Holding -> Checking" << std::endl;
      _state = Rechecking;
      _checkTimer.start();
   }
}

void PickUpCube::ProcessTags()
{
   _distanceToTarget = OUT_OF_RANGE;

   for(auto tag : _sensors->GetTags())
   {
      if(tag.GetId() != 0) continue;

      switch(_state)
      {
      case NotHolding:
         if(fabs(tag.Alignment()) < ALIGNMENT_THRESHOLD
            && tag.HorizontalDistance() < PICKUP_DISTANCE)
         {
            _state = LastInch;
         }
         break;
      case Checking:
         if(fabs(tag.Alignment()) < 0.01)
         {
            _distanceToTarget = tag.Distance();
         }
         break;
      default:
         break;
      }
   }
}

void PickUpCube::PickupTimeout(const ros::TimerEvent& event)
{
   _allowReset = true;
   switch(_state)
   {
   case LastInch:
      _state = Grip;
      break;
   case Grip:
      _state = Raise;
      break;
   case Raise:
      _checkTimer.stop();
      _checkTimer.setPeriod(ros::Duration(3), true);
      std::cout << "Raise -> Checking" << std::endl;
      _state = Checking;
      _checkTimer.start();
      break;
   }
}

void PickUpCube::ResetTimer(double duration)
{
   if(!_allowReset) return;
   _allowReset = false;
   _pickupTimer.stop();
   _pickupTimer.setPeriod(ros::Duration(duration));
   _pickupTimer.start();
}

void PickUpCube::Update()
{
   ProcessTags();
   _centerRange = _sensors->GetCenterSonar();
   _action = _llAction;

   switch(_state)
   {
   case LastInch:
      std::cout << "in LastInch" << std::endl;
      _action.drive.left = 60;
      _action.drive.right = 60;
      ResetTimer(0.7);
      break;
   case Grip:
      _action.drive.left = 0;
      _action.drive.right = 0;
      _action.grip = GripperControl::CLOSED;
      _action.wrist = WristControl::DOWN;
      ResetTimer(1.5);
      break;
   case Raise:
      _action.drive.left = 0;
      _action.drive.right = 0;
      _action.grip = GripperControl::CLOSED;
      _action.wrist = WristControl::UP;
      ResetTimer(2);
      break;
   case Holding:
      _action = _subsumedBehavior->GetAction();
      _action.grip = GripperControl::CLOSED;
      _action.wrist = WristControl::DOWN_2_3;
      break;
   case Checking:
      _action.drive.left = 0;
      _action.drive.right = 0;
   case Rechecking:
      _action.grip = GripperControl::CLOSED;
      _action.wrist = WristControl::UP;
      break;
   default:
      break;
   }
}