import java.io.*;
import java.util.*;
import info.openrocket.core.startup.OpenRocketCore;
import info.openrocket.core.file.GeneralRocketLoader;
import info.openrocket.core.document.*;
import info.openrocket.core.rocketcomponent.*;
import info.openrocket.core.simulation.*;
import info.openrocket.core.simulation.listeners.AbstractSimulationListener;
import info.openrocket.core.models.wind.PinkNoiseWindModel;
import info.openrocket.core.util.*;

// Read a copy in memory; never writes or modifies the user's .ork.
// Suppress recovery for an independent decision replay, not a closed-loop deployment simulation.
public class ExportFlight {
  public static void main(String[] a) throws Exception {
    System.setProperty("java.awt.headless","true");
    System.setProperty("openrocket.bypass.presets","true");OpenRocketCore.initialize();
    OpenRocketDocument doc=new GeneralRocketLoader(new File(a[0])).load();Simulation sim=doc.getSimulation(0);
    for(RocketComponent c:doc.getRocket()) if(c instanceof RecoveryDevice)
      ((RecoveryDevice)c).getDeploymentConfigurations().get(sim.getFlightConfigurationId()).setDeployEvent(DeploymentConfiguration.DeployEvent.NEVER);
    sim.getOptions().setTimeStep(0.01);sim.getOptions().setMaxSimulationTime(30);
    sim.getOptions().setRandomSeed(20261017);
    PinkNoiseWindModel wind=new PinkNoiseWindModel(20261017);
    wind.setAverage(sim.getOptions().getWindSpeedAverage());wind.setStandardDeviation(sim.getOptions().getWindSpeedDeviation());wind.setDirection(sim.getOptions().getWindDirection());
    try(PrintWriter pw=new PrintWriter(a[1])) {
      pw.println("time_s,altitude_m,vz_m_s,ax_world,ay_world,az_world,qw,qx,qy,qz,gravity,pressure_pa");
      sim.simulate(new AbstractSimulationListener(){
        Coordinate acc=new Coordinate(0,0,0);
        public boolean isSystemListener(){return true;}
        public void startSimulation(SimulationStatus st){st.getSimulationConditions().setWindModel(wind);}
        public AccelerationData postAccelerationCalculation(SimulationStatus st,AccelerationData d){acc=d.getLinearAccelerationWC();return d;}
        public void postStep(SimulationStatus st){
          Quaternion q=st.getRocketOrientationQuaternion();
          double g=st.getSimulationConditions().getGravityModel().getGravity(st.getRocketWorldPosition());
          double p=st.getSimulationConditions().getAtmosphericModel().getConditions(st.getRocketWorldPosition().getAltitude()).getPressure();
          Coordinate av=st.isLiftoff()?acc:new Coordinate(0,0,0);
          pw.printf(Locale.ROOT,"%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.12f,%.12f,%.12f,%.12f,%.9f,%.9f%n",
            st.getSimulationTime(),st.getRocketPosition().z,st.getRocketVelocity().z,av.x,av.y,av.z,q.getW(),q.getX(),q.getY(),q.getZ(),g,p);
        }
      });
    }
    System.out.println("APOGEE="+sim.getSimulatedData().getMaxAltitude()+" WARNINGS="+sim.getSimulatedWarnings());
    System.exit(0);
  }
}
